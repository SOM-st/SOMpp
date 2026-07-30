#include "CopyingCollector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../misc/defs.h"  // NOLINT(misc-include-cleaner)
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "CopyingHeap.h"

static gc_oop_t copy_if_necessary(gc_oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(IsValidObject(obj));

    size_t const gcField = obj->GetGCField();
    // GCField is used as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0) {
        return (gc_oop_t)gcField;
    }

    assert(GetHeap<CopyingHeap>()->IsInOldBufferAndOldBufferIsValid(obj));
    assert(!obj->IsMarkedInvalid());

    // we have to clone ourselves
    AbstractVMObject* newObj = obj->CloneForMovingGC();

    assert(GetHeap<CopyingHeap>()->IsInCurrentBuffer(newObj));

    if (DEBUG) {
        obj->MarkObjectAsInvalid();
    }

    obj->SetGCField((intptr_t)newObj);
    return tmp_ptr(newObj);
}

void CopyingCollector::Collect() {
    DebugLog("CopyGC Collect\n");

    Timer::GCTimer.Resume();
    // reset collection trigger
    heap->resetGCTrigger();

    static bool increaseMemory;
    heap->switchBuffers(increaseMemory);
    increaseMemory = false;

    Universe::WalkGlobals(copy_if_necessary);

    // now copy all objects that are referenced by the objects we have moved so
    // far
    auto* curObject = (AbstractVMObject*)(heap->currentBuffer);
    while (curObject < heap->nextFreePosition) {
        curObject->WalkObjects(copy_if_necessary);
        curObject =
            (AbstractVMObject*)((size_t)curObject + curObject->GetObjectSize());
    }

    heap->invalidateOldBuffer();

    // if semispace is still 50% full after collection, we have to realloc
    // bigger ones -> done in next collection
    if ((size_t)(heap->nextFreePosition) - (size_t)(heap->currentBuffer) >=
        (size_t)(heap->currentBufferEnd) - (size_t)(heap->nextFreePosition)) {
        increaseMemory = true;
    }

    Timer::GCTimer.Halt();
}
