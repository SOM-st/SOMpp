#include "GenerationalHeap.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMObjectBase.h"
#include "GenerationalCollector.h"
#include "Heap.h"

using namespace std;

GenerationalHeap::GenerationalHeap(size_t objectSpaceSize)
    : Heap<GenerationalHeap>(new GenerationalCollector(this)),
      nursery(malloc(objectSpaceSize)),
      nursery_end((size_t)nursery + objectSpaceSize),
      nurserySize(objectSpaceSize), maxNurseryObjSize(objectSpaceSize / 2),
      nextFreePosition(nursery),

      // our initial collection limit is 90% of objectSpaceSize
      collectionLimit((void*)((size_t)nursery +
                              ((size_t)((double)objectSpaceSize * 0.9)))) {
    memset(nursery, 0x0, objectSpaceSize);
}

void* GenerationalHeap::AllocateNurseryObject(size_t size) {
    // if allocation does not fit, attempt allocation in old generation
    if (size > maxNurseryObjSize ||
        size > nursery_end - (size_t)nextFreePosition) {
        return AllocateMatureObject(size);
    }

    void* newObject = nextFreePosition;
    nextFreePosition =
        static_cast<void*>(static_cast<char*>(nextFreePosition) + size);
    // let's see if we have to trigger the GC
    if (nextFreePosition > collectionLimit || gcStressMode) {
        requestGC();
    }
    return newObject;
}

void* GenerationalHeap::AllocateMatureObject(size_t size) {
    void* newObject = malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    allocatedObjects.push_back(static_cast<AbstractVMObject*>(newObject));
    matureObjectsSize += size;
    if (gcStressMode) {
        requestGC();
    }
    return newObject;
}

void GenerationalHeap::writeBarrier_OldHolder(VMObjectBase* holder,
                                              vm_oop_t referencedObject) {
    if (IsObjectInNursery(referencedObject)) {
        oldObjsWithRefToYoungObjs.push_back((size_t)holder);
        holder->SetGCField(holder->GetGCField() | MASK_SEEN_BY_WRITE_BARRIER);
    }
}
