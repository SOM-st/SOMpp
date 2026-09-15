#pragma once

#include <cstddef>

#include "DebugCopyingCollector.h"
#include "Heap.h"

class DebugCopyingHeap : public Heap<DebugCopyingHeap> {
    friend class DebugCopyingCollector;

public:
    explicit DebugCopyingHeap(size_t objectSpaceSize)
        : Heap<DebugCopyingHeap>(new DebugCopyingCollector(this)),
          currentHeapSize(objectSpaceSize),
          collectionLimit((size_t)((double)objectSpaceSize * 0.9)) {}

    void* AllocateObject(size_t size);

    bool IsInCurrentBuffer(AbstractVMObject* obj);
    bool IsInOldBufferAndOldBufferIsValid(AbstractVMObject* obj);

private:
    void switchBuffers(bool increaseMemory);
    void invalidateOldBuffer();

    vector<AbstractVMObject*> currentHeap;
    size_t currentHeapSize;
    size_t currentHeapUsage{0};
    size_t collectionLimit;

    vector<AbstractVMObject*> oldHeap;
    size_t oldHeapSize{0};

    bool oldHeapIsValid{false};
};
