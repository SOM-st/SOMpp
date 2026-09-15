#pragma once

#include <cstring>

#include "Heap.h"

class CopyingHeap : public Heap<CopyingHeap> {
    friend class CopyingCollector;

public:
    explicit CopyingHeap(size_t objectSpaceSize);
    void* AllocateObject(size_t size);

    bool IsInCurrentBuffer(AbstractVMObject* obj);
    bool IsInOldBufferAndOldBufferIsValid(AbstractVMObject* obj);

private:
    void switchBuffers(bool increaseMemory);
    void invalidateOldBuffer();

    void* currentBuffer;
    void* collectionLimit;
    void* currentBufferEnd;

    void* oldBuffer;
    void* oldBufferEnd;

    void* nextFreePosition;
    bool oldBufferIsValid{false};
};
