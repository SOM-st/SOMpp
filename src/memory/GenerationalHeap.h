#pragma once

#include <cassert>

#include "../misc/defs.h"
#include "../vm/IsValidObject.h"
#include "../vmobjects/VMObjectBase.h"
#include "Heap.h"

#ifdef UNITTESTS
struct VMObjectCompare {
    bool operator()(pair<vm_oop_t, vm_oop_t> lhs, pair<vm_oop_t, vm_oop_t> rhs)
        const {
        return (size_t)lhs.first < (size_t)rhs.first &&
               (size_t)lhs.second < (size_t)rhs.second;
    }
};
#endif

class GenerationalHeap : public Heap<GenerationalHeap> {
    friend class GenerationalCollector;

public:
    explicit GenerationalHeap(size_t objectSpaceSize);
    void* AllocateNurseryObject(size_t size);
    void* AllocateMatureObject(size_t size);
    void writeBarrier(VMObjectBase* holder, vm_oop_t referencedObject);
    [[nodiscard]] inline bool IsObjectInNursery(vm_oop_t obj) const;
    [[nodiscard]] inline bool IsInNursery(const void* ptr) const;
#ifdef UNITTESTS
    std::set<pair<vm_oop_t, vm_oop_t>, VMObjectCompare> writeBarrierCalledOn;
#endif
private:
    void* nursery;
    size_t nursery_end;
    size_t nurserySize;
    size_t maxNurseryObjSize;
    size_t matureObjectsSize{0};
    size_t majorCollectionThreshold{(size_t)5 * 1024U * 1024U};  // 5 MB
    void* nextFreePosition;
    void writeBarrier_OldHolder(VMObjectBase* holder,
                                vm_oop_t referencedObject);
    void* collectionLimit;
    vector<size_t> oldObjsWithRefToYoungObjs;
    vector<AbstractVMObject*> allocatedObjects;
};

inline bool GenerationalHeap::IsObjectInNursery(vm_oop_t obj) const {
    assert(IsValidObject(obj));

    return IsInNursery(obj);
}

inline bool GenerationalHeap::IsInNursery(const void* ptr) const {
    return (size_t)ptr >= (size_t)nursery && (size_t)ptr < nursery_end;
}

inline void GenerationalHeap::writeBarrier(VMObjectBase* holder,
                                           vm_oop_t referencedObject) {
#ifdef UNITTESTS
    writeBarrierCalledOn.insert(make_pair(holder, referencedObject));
#endif

    assert(IsValidObject(referencedObject));
    assert(IsValidObject(holder));

    const size_t gcField = *(((size_t*)holder) + 1);
    if ((gcField & 6U /* MASK_OBJECT_IS_OLD + MASK_SEEN_BY_WRITE_BARRIER */) ==
        2U /* MASK_OBJECT_IS_OLD */) {
        writeBarrier_OldHolder(holder, referencedObject);
    }
}
