#include "MarkSweepCollector.h"

#include <cassert>
#include <cstddef>
#include <vector>

#include "../misc/debug.h"
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "MarkSweepHeap.h"

static size_t markedBytes = 0;
static std::vector<AbstractVMObject*> markStack;

static gc_oop_t mark_object(gc_oop_t oop) {
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(IsValidObject(obj));

    if (obj->GetGCField() == MarkSweepHeap::GC_MARKED) {
        return oop;
    }

    obj->SetGCField(MarkSweepHeap::GC_MARKED);
    markedBytes += MarkSweepCollector::GetAllocationSize(obj->GetObjectSize());
    markStack.push_back(obj);
    return oop;
}

void MarkSweepCollector::Collect() {
    DebugLog("MarkSweep Collect\n");

    Timer::GCTimer.Resume();
    heap->resetGCTrigger();

    markReachableObjects();
    heap->sweep();

    Timer::GCTimer.Halt();
}

void MarkSweepCollector::markReachableObjects() {
    markedBytes = 0;
    markStack.clear();

    Universe::WalkGlobals(mark_object);

    while (!markStack.empty()) {
        AbstractVMObject* obj = markStack.back();
        markStack.pop_back();
        obj->WalkObjects(mark_object);
    }

    heap->liveBytes = markedBytes;
}
