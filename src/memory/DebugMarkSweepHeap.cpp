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
      // our initial collection limit is 90% of objectSpaceSize
      collectionLimit((size_t)((double)objectSpaceSize * 0.9)) {}

DebugMarkSweepHeap::~DebugMarkSweepHeap() {
    // free the tracked heap so leak checkers report only genuine leaks
    for (auto* obj : *allocatedObjects) {
        free(obj);
    }
    delete allocatedObjects;
}

AbstractVMObject* DebugMarkSweepHeap::AllocateObject(size_t size) {
    auto* newObject = (AbstractVMObject*)malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    spcAlloc += size;
    memset((void*)newObject, 0, size);
    // AbstractObjects (Integer,...) have no Size field anymore -> set within
    // VMObject's new operator
    allocatedObjects->push_back(newObject);
    // let's see if we have to trigger the GC
    if (spcAlloc >= collectionLimit) {
        requestGC();
    }
    return newObject;
}
