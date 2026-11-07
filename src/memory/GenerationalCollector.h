#pragma once

#include "../misc/defs.h"
#include "GarbageCollector.h"

class GenerationalHeap;
class GenerationalCollector : public GarbageCollector<GenerationalHeap> {
public:
    explicit GenerationalCollector(GenerationalHeap* heap);
    void Collect() override;

private:
    void MajorCollection();
    void MinorCollection();
};
