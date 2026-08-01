#pragma once

#include "GarbageCollector.h"

class DebugCopyingHeap;

class DebugCopyingCollector : public GarbageCollector<DebugCopyingHeap> {
public:
    explicit DebugCopyingCollector(DebugCopyingHeap* h)
        : GarbageCollector(h) {};

    void Collect() override;
};
