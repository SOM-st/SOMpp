#pragma once

#include "../misc/defs.h"
#include "Heap.h"

// Debug-only Mark/Sweep heap: the simple reference implementation that
// malloc()s/free()s every object and tracks them in a side list.
class DebugMarkSweepHeap : public Heap<DebugMarkSweepHeap> {
    friend class DebugMarkSweepCollector;

public:
    explicit DebugMarkSweepHeap(size_t objectSpaceSize = 1048576);
    ~DebugMarkSweepHeap();
    AbstractVMObject* AllocateObject(size_t size);

private:
    vector<AbstractVMObject*>* allocatedObjects;
    size_t spcAlloc{0};
    size_t collectionLimit;
};
