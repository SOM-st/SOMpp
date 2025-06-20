#include "Vector.h"

#include <cstddef>
#include <cstdint>

#include "../misc/defs.h"
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
    int64_t const size = INT_VAL(arg);
    return Universe::NewVector(size, static_cast<VMClass*>(clazz));
}

static vm_oop_t vecAt(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = INT_VAL(arg);
    return self->GetStorage(index);
}

static vm_oop_t vecAtPut(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = INT_VAL(at);         // Set the index looking for
    // Call method to set the value at index. That deals with 1to0 indexing
    // conversion
    return self->SetStorage(index, put);
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

    // These bytecode hashes may need to be updated occationally
    // If they need to be updated, the corresponding primitive implementations
    // likely need to be adapted, too.

    Add("at:", false, &vecAt, 15701225123643718540ULL, &vecAt,
        6812223825622950441ULL);
    Add("at:put:", false, &vecAtPut, 15701225123643718540ULL, &vecAtPut,
        1424373843580326011ULL);

    Add("first", &vecFirst, false, 10026279788538546667ULL);
    Add("last", &vecLast, false, 12704268779516119438ULL);
    Add("append:", &vecAppend, false, 13118885541413071290ULL);
    Add("remove", &removeLast, false, 1325639674086910439ULL);
    Add("remove:", &removeObject, false, 18423337479191764055ULL);
    Add("size", &vecSize, false, 275685630939944626ULL);
    Add("capacity", &capacity, false, 12474412091446719372ULL);
    Add("asArray", &asArray, false, 10354287759150596565ULL);
    Add("removeFirst", false, &removeFirst, 10589921408041234885ULL,
        &removeFirst, 14759376054960058883ULL);
    Add("removeAll", &removeAll, false, 13065396693477551925ULL);
};
