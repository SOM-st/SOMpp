#include "Vector.h"

#include <cstddef>
#include <cstdint>
#include <iostream>

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
    return self->SetIndexableField(index, put);
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
    return self->StorageArray();
}

/* Handles initialization of primitives separately to object creation */
/* Object is created when statics are loaded and no .som file has been read */
void _Vector::LateInitialize(size_t hash) {
#ifdef USE_VECTOR_PRIMITIVES
    // Hashes of the class which can be used to determine source code
    // Both hashes computed using std::hash<std::string>
    const size_t stdVecHash = 6847463072365130734;   // Correct as of 04/06/2025
    const size_t awfyVecHash = 4760964668761413413;  // Correct as of 04/06/2025

    cout << "Vector Source Hash" << hash << "\n";

    // Install implementation specific methods
    if (hash == stdVecHash) {
        cout << "Vector: Loading core-lib vector primitives." << "\n";

        Add("first", &vecFirst, false);
        Add("last", &vecLast, false);
        Add("remove", &removeLast, false);
        Add("asArray", &asArray, false);

    } else if (hash == awfyVecHash) {
        cout << "Vector: Loading awfy vector primitives. " << "\n";

        Add("removeAll", &removeAll, false);
        Add("capacity", &capacity, false);
    }

    // Install implementation independent methods (For AWFY or core-lib)
    if (hash == stdVecHash || hash == awfyVecHash) {
        cout << "Vector: Installing independent Primitives. " << "\n";

        Add("new", &vecNew, true);
        Add("new:", &vecNewSize, true);
        Add("at:", &vecAt, false);
        Add("at:put:", &vecAtPut, false);
        Add("append:", &vecAppend, false);
        Add("removeFirst", &removeFirst, false);
        Add("remove:", &removeObject, false);
        Add("size", &vecSize, false);

    } else {
        cout << "Vector: Unknown Vector hash. No primitive methods installed"
             << "\n";
    }
#endif
}

_Vector::_Vector() = default;