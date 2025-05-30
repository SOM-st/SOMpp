#include "Vector.h"

#include <cstdint>

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
    auto* classPtr = static_cast<VMClass*>(clazz);
    int64_t const size = INT_VAL(arg);
    return Universe::NewVector(size, static_cast<VMClass*>(clazz));
}

static vm_oop_t vecAt(vm_oop_t obj, vm_oop_t arg) {
    auto* self = static_cast<VMVector*>(obj);
    int64_t const index = INT_VAL(arg);
    return self->GetIndexableField(index);
}

static vm_oop_t vecAtPut(vm_oop_t obj, vm_oop_t at, vm_oop_t put) {
    auto* self = static_cast<VMVector*>(obj);  // Cast itself as a VMVector
    int64_t const index = INT_VAL(at);         // Set the index looking for
    // Call method to set the value at index. That deals with 1to0 indexing
    // conversion
    self->SetIndexableField(index, put);
    return put;  // Return the value that was set
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

static vm_oop_t contains(vm_oop_t obj, vm_oop_t other) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Contains(other);
}

static vm_oop_t indexOf(vm_oop_t obj, vm_oop_t other) {
    auto* self = static_cast<VMVector*>(obj);
    return self->IndexOf(other);
}

static vm_oop_t vecSize(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->Size();
}

static vm_oop_t asArray(vm_oop_t obj) {
    auto* self = static_cast<VMVector*>(obj);
    return self->StorageArray();
}

_Vector::_Vector() {
    Add("new", &vecNew, true);
    Add("new:", &vecNewSize, true);
    Add("at:", &vecAt, false);
    Add("at:put:", &vecAtPut, false);
    Add("append:", &vecAppend, false);
    Add("first", &vecFirst, false);
    Add("last", &vecLast, false);
    Add("remove", &removeLast, false);
    Add("removeFirst", &removeFirst, false);
    Add("contains:", &contains, false);
    Add("indexOf:", &indexOf, false);
    Add("size", &vecSize, false);
    Add("remove:", &removeObject, false);
    Add("asArray", &asArray, false);
}