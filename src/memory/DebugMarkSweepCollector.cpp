#include "DebugMarkSweepCollector.h"

#include <cstddef>
#include <vector>

#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "DebugMarkSweepHeap.h"

#define GC_MARKED 3456

void DebugMarkSweepCollector::Collect() {
    DebugLog("DebugMarkSweep Collect\n");

    auto* heap = GetHeap<DebugMarkSweepHeap>();
    Timer::GCTimer.Resume();
    heap->resetGCTrigger();

    markReachableObjects();

    auto* survivors = new vector<AbstractVMObject*>();
    size_t survivorsSize = 0;

    vector<AbstractVMObject*>::iterator iter;
    for (iter = heap->allocatedObjects->begin();
         iter != heap->allocatedObjects->end();
         iter++) {
        if ((*iter)->GetGCField() == GC_MARKED) {
            // object ist marked -> let it survive
            survivors->push_back(*iter);
            survivorsSize += (*iter)->GetObjectSize();
            (*iter)->SetGCField(0);
        } else {
            // not marked -> kill it
            heap->FreeObject(*iter);
        }
    }

    delete heap->allocatedObjects;
    heap->allocatedObjects = survivors;

    heap->spcAlloc = survivorsSize;
    heap->collectionLimit = 2 * survivorsSize;
    Timer::GCTimer.Halt();
}

static gc_oop_t mark_object(gc_oop_t oop) {
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);

    if (obj->GetGCField() != 0) {
        return oop;
    }

    obj->SetGCField(GC_MARKED);
    obj->WalkObjects(mark_object);
    return oop;
}

void DebugMarkSweepCollector::markReachableObjects() {
    Universe::WalkGlobals(mark_object);
}
