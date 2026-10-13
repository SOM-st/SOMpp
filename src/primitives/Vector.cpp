#include "Vector.h"

#include <cstdint>

#include "../misc/defs.h"  // NOLINT(misc-include-cleaner)
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMVector.h"

static vm_oop_t vecNew(vm_oop_t clazz) {
    auto* classPtr = static_cast<VMClass*>(clazz);
    int64_t const size = 50;
    VMVector* vec = Universe::NewVector(size, classPtr);
    return vec;
}

static vm_oop_t vecNewSize(vm_oop_t clazz, vm_oop_t arg) {
    int64_t const size = SMALL_INT_VAL(arg);
    return Universe::NewVector(size, static_cast<VMClass*>(clazz));
}

static vm_oop_t vecAt(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = SMALL_INT_VAL(arg);
    return self->GetStorage(index);
}

static vm_oop_t vecAtAWFY(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = SMALL_INT_VAL(arg);
    return self->AWFYGetStorage(index);
}

static vm_oop_t vecAtPut(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = SMALL_INT_VAL(at);   // Set the index looking for
    // Call method to set the value at index. That deals with 1to0 indexing
    // conversion
    return self->SetStorage(index, put);
}

static vm_oop_t vecAtPutAWFY(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = SMALL_INT_VAL(at);   // Set the index looking for
    // Call method to set the value at index. That does not deal with 1to0
    // indexing conversion
    self->SetStorageAWFY(index, put);
    return self;
}

static vm_oop_t vecAppend(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    self->Append(arg);
    return self;
}

static vm_oop_t vecFirst(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetFirst();
}

static vm_oop_t vecLast(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->GetLast();
}

static vm_oop_t removeLast(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveLast();
}

static vm_oop_t removeFirst(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveFirst();
}

static vm_oop_t removeObject(vm_oop_t obj, vm_oop_t other) {
    auto* self = static_cast<VMVector*>(obj);
    return self->RemoveObj(other);
}

static vm_oop_t removeAll(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    self->RemoveAll();
    return self;
}

static vm_oop_t vecSize(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Size();
}

static vm_oop_t capacity(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Capacity();
}

static vm_oop_t asArray(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->copyStorageArray();
}

_Vector::_Vector() {
    if (!USE_VECTOR_PRIMITIVES) {
        return;
    }

    // These bytecode hashes may need to be updated occasionally
    // If they need to be updated, the corresponding primitive implementations
    // likely need to be adapted, too.
    // In order to print out hashes use the -prim-hash-check flag when executing
    // SOM++

    Add("at:", false, &vecAt, 1236943569, &vecAtAWFY, 3362920797);
    Add("at:put:", false, &vecAtPut, 1236943569, &vecAtPutAWFY, 3362920797);

    Add("first", &vecFirst, false, 1780830677);
    Add("last", &vecLast, false, 1777166132);
    Add("append:", &vecAppend, false, 3874135055);
    Add("remove", &removeLast, false, 3484229321);
    Add("remove:", &removeObject, false, 2357433328);
    Add("size", &vecSize, false, 3985266107);
    Add("capacity", &capacity, false, 628037838);
    Add("asArray", &asArray, false, 746407897);
    Add("removeFirst", &removeFirst, false, 2050519951);
    Add("removeAll", &removeAll, false, 1510429688);
};
