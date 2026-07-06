#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class MarkSweepHeap;
class MarkSweepCollector : public GarbageCollector<MarkSweepHeap> {
public:
    explicit MarkSweepCollector(MarkSweepHeap* heap) : GarbageCollector(heap) {}
    void Collect() override;

private:
    static void markReachableObjects();
};
