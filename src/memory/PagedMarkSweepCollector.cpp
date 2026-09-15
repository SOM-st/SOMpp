#include "PagedMarkSweepCollector.h"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "PagedMarkSweepHeap.h"

size_t PagedMarkSweepCollector::epoch = 0;
size_t PagedMarkSweepCollector::markedBytes = 0;
std::vector<AbstractVMObject*> PagedMarkSweepCollector::markStack;

void PagedMarkSweepCollector::Collect() {
    DebugLog("PagedMarkSweep Collect\n");

    auto* heap = GetHeap<PagedMarkSweepHeap>();
    Timer::GCTimer.Resume();
    heap->resetGCTrigger();

    // Start a new cycle, epoch is only compared by identity
    // so wrap around is not an issue.
    heap->epoch += 1;
    epoch = heap->epoch;
    markedBytes = 0;

    // Drop all free lists.
    // This also guarantees that we do not allocate into pages
    // that still need to be swept, which is important because
    // sweeping is lazy.
    for (auto& list : heap->freeLists) {
        list = nullptr;
    }
    for (auto& cursor : heap->sweepCursor) {
        cursor = 0;
    }

    markReachableObjects();

    // Sweep the large objects eagerly
    std::vector<AbstractVMObject*> survivingLarge;
    for (auto* obj : heap->largeObjects) {
        if (obj->GetGCField() == epoch) {
            survivingLarge.push_back(obj);
        } else {
            heap->FreeObject(obj);
        }
    }
    heap->largeObjects = std::move(survivingLarge);

    heap->spcAlloc = markedBytes;
    size_t const grown = 2 * markedBytes;
    heap->collectionLimit =
        grown > heap->minCollectionLimit ? grown : heap->minCollectionLimit;
    Timer::GCTimer.Halt();
}

static gc_oop_t mark_object(gc_oop_t oop) {
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(IsValidObject(obj));

    if (obj->GetGCField() == PagedMarkSweepCollector::epoch) {
        return oop;
    }

    obj->SetGCField(PagedMarkSweepCollector::epoch);
    PagedMarkSweepCollector::markedBytes += obj->GetObjectSize();
    PagedMarkSweepCollector::markStack.push_back(obj);
    return oop;
}

void PagedMarkSweepCollector::markReachableObjects() {
    markStack.clear();
    Universe::WalkGlobals(mark_object);

    while (!markStack.empty()) {
        AbstractVMObject* obj = markStack.back();
        markStack.pop_back();
        obj->WalkObjects(mark_object);
    }
}
