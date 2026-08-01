#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class GenerationalHeap;
class GenerationalCollector : public GarbageCollector<GenerationalHeap> {
public:
    explicit GenerationalCollector(GenerationalHeap* heap);
    void Collect() override;

private:
    uintptr_t majorCollectionThreshold{(uintptr_t)5 * 1024U * 1024U};  // 5 MB
    size_t matureObjectsSize{0};
    void MajorCollection();
    void MinorCollection();
};
