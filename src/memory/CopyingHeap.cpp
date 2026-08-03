#include "CopyingHeap.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../misc/defs.h"  // NOLINT(misc-include-cleaner)
#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "CopyingCollector.h"
#include "Heap.h"

CopyingHeap::CopyingHeap(size_t objectSpaceSize)
    : Heap<CopyingHeap>(new CopyingCollector(this)),
      currentBuffer(malloc(objectSpaceSize)),
      oldBuffer(malloc(objectSpaceSize)),
      currentBufferEnd((void*)((size_t)currentBuffer + objectSpaceSize)),
      collectionLimit((void*)((size_t)currentBuffer +
                              ((size_t)((double)objectSpaceSize * 0.9)))),
      oldBufferEnd((void*)((size_t)oldBuffer + objectSpaceSize)),
      nextFreePosition(currentBuffer) {
    memset(currentBuffer, 0x0, objectSpaceSize);
    memset(oldBuffer, 0x0, objectSpaceSize);
}

void CopyingHeap::switchBuffers(bool increaseMemory) {
    size_t const oldBufSizeBeforeSwitch =
        (size_t)oldBufferEnd - (size_t)oldBuffer;

    void* oldBufferBeforeSwitch = oldBuffer;
    void* oldBufferEndBeforeSwitch = oldBufferEnd;

    // make current buffer the old one
    oldBuffer = currentBuffer;
    oldBufferEnd = currentBufferEnd;
    oldBufferIsValid = true;

    size_t currentBufSize = 0;
    if (increaseMemory) {
        // increase memory if scheduled in collection before
        free(oldBufferBeforeSwitch);
        oldBufferEndBeforeSwitch = nullptr;

        size_t const newSize = oldBufSizeBeforeSwitch * 2;
        currentBuffer = malloc(newSize);

        if (currentBuffer == nullptr) {
            ErrorExit("unable to allocate heap memory");
        }

        currentBufferEnd = (void*)((size_t)currentBuffer + newSize);
        collectionLimit =
            (void*)((size_t)currentBuffer + ((size_t)((double)newSize * 0.9)));
        nextFreePosition = currentBuffer;
        currentBufSize = newSize;
    } else {
        currentBuffer = oldBufferBeforeSwitch;
        currentBufferEnd = oldBufferEndBeforeSwitch;
        currentBufSize = oldBufSizeBeforeSwitch;

        collectionLimit =
            (void*)((size_t)oldBufferBeforeSwitch +
                    (size_t)((double)oldBufSizeBeforeSwitch * 0.9));
        nextFreePosition = currentBuffer;
    }

    // init currentBuffer with zeros
    memset(currentBuffer, 0x0, currentBufSize);
}

void CopyingHeap::invalidateOldBuffer() {
    oldBufferIsValid = false;

    size_t const currentBufSize =
        (size_t)currentBufferEnd - (size_t)currentBuffer;
    size_t const oldBufSize = (size_t)oldBufferEnd - (size_t)oldBuffer;

    if (DEBUG) {
        memset(oldBuffer, 0xFF, oldBufSize);
    }

    if (currentBufSize > oldBufSize) {
        free(oldBuffer);

        oldBuffer = malloc(currentBufSize);

        if (oldBuffer == nullptr) {
            ErrorExit("unable to allocate heap memory");
        }

        oldBufferEnd = (void*)((size_t)oldBuffer + currentBufSize);
    }
}

AbstractVMObject* CopyingHeap::AllocateObject(size_t size) {
    auto* newObject = (AbstractVMObject*)nextFreePosition;
    nextFreePosition = (void*)((size_t)nextFreePosition + size);
    if (nextFreePosition > currentBufferEnd) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }

    // let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit) {
        requestGC();
    }

    return newObject;
}

bool CopyingHeap::IsInCurrentBuffer(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    auto const objAddress = (size_t)obj;
    return (size_t)currentBuffer <= objAddress &&
           objAddress < (size_t)currentBufferEnd;
}

bool CopyingHeap::IsInOldBufferAndOldBufferIsValid(AbstractVMObject* obj) {
    if (obj == nullptr) {
        return true;
    }

    if (!oldBufferIsValid) {
        assert(oldBufferIsValid);
        return false;
    }

    auto const objAddress = (size_t)obj;
    assert((size_t)oldBuffer <= objAddress);
    assert(objAddress < (size_t)oldBufferEnd);

    return (size_t)oldBuffer <= objAddress && objAddress < (size_t)oldBufferEnd;
}
