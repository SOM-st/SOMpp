#pragma once

#include <vector>

#include "../misc/defs.h"
#include "GarbageCollector.h"

class PagedMarkSweepHeap;
class PagedMarkSweepCollector : public GarbageCollector<PagedMarkSweepHeap> {
public:
    explicit PagedMarkSweepCollector(PagedMarkSweepHeap* heap)
        : GarbageCollector(heap) {}
    void Collect() override;

private:
    static void markReachableObjects();

public:
    static size_t epoch;
    static size_t markedBytes;
    static std::vector<AbstractVMObject*> markStack;
};
