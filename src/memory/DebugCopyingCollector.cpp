
#include "DebugCopyingCollector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "../memory/Heap.h"
#include "../misc/Timer.h"
#include "../misc/debug.h"
#include "../misc/defs.h"  // NOLINT(misc-include-cleaner)
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "DebugCopyingHeap.h"

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

    assert(GetHeap<DebugCopyingHeap>()->IsInOldBufferAndOldBufferIsValid(obj));
    assert(!obj->IsMarkedInvalid());

    // we have to clone ourselves
    AbstractVMObject* newObj = obj->CloneForMovingGC();

    assert(GetHeap<DebugCopyingHeap>()->IsInCurrentBuffer(newObj));

    if (DEBUG) {
        obj->MarkObjectAsInvalid();
    }

    obj->SetGCField((uintptr_t)newObj);
    return tmp_ptr(newObj);
}

void DebugCopyingCollector::Collect() {
    DebugLog("DebugCopyGC Collect\n");

    // we assume the old heap is empty, because we want to switch to it
    assert(heap->oldHeap.empty());

    Timer::GCTimer.Resume();
    // reset collection trigger
    heap->resetGCTrigger();

    static bool increaseMemory;
    heap->switchBuffers(increaseMemory);
    increaseMemory = false;

    Universe::WalkGlobals(copy_if_necessary);

    // now copy all objects that are referenced by the objects we have moved so
    // far
    for (size_t i = 0; i < heap->currentHeap.size(); i += 1) {
        heap->currentHeap.at(i)->WalkObjects(copy_if_necessary);
    }

    heap->invalidateOldBuffer();

    // if semispace is still 50% full after collection, we have to realloc
    // bigger ones -> done in next collection
    size_t const freeSpace = heap->currentHeapSize - heap->currentHeapUsage;
    if (heap->currentHeapUsage > freeSpace) {
        increaseMemory = true;
    }

    Timer::GCTimer.Halt();
}
