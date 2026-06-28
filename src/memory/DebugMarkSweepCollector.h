#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class DebugMarkSweepHeap;
class DebugMarkSweepCollector : public GarbageCollector<DebugMarkSweepHeap> {
public:
    explicit DebugMarkSweepCollector(DebugMarkSweepHeap* heap)
        : GarbageCollector(heap) {}
    void Collect() override;

private:
    static void markReachableObjects();
};
