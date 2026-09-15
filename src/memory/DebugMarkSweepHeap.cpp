#include "DebugMarkSweepHeap.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../memory/Heap.h"
#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "DebugMarkSweepCollector.h"

DebugMarkSweepHeap::DebugMarkSweepHeap(size_t objectSpaceSize)
    : Heap<DebugMarkSweepHeap>(new DebugMarkSweepCollector(this)),
      allocatedObjects(new vector<AbstractVMObject*>()),
      collectionLimit((size_t)((double)objectSpaceSize * 0.9)) {}

DebugMarkSweepHeap::~DebugMarkSweepHeap() {
    // free the tracked heap so leak checkers report only genuine leaks
    for (auto* obj : *allocatedObjects) {
        free(obj);
    }
    delete allocatedObjects;
}

void* DebugMarkSweepHeap::AllocateObject(size_t size) {
    void* newObject = malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    spcAlloc += size;
    memset(newObject, 0, size);
    allocatedObjects->push_back(static_cast<AbstractVMObject*>(newObject));

    // let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit || gcStressMode) {
        requestGC();
    }
    return newObject;
}
