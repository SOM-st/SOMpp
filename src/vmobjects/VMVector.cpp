#include "../vmobjects/VMVector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "../interpreter/Interpreter.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"

const size_t VMVector::VMVectorNumberOfFields = 3;

VMVector::VMVector(vm_oop_t first, vm_oop_t last, VMArray* storage)
    : VMObject(VMVectorNumberOfFields, sizeof(VMVector)),
      first(store_with_separate_barrier(first)),
      last(store_with_separate_barrier(last)),
      storage(store_with_separate_barrier(storage)) {
    static_assert(VMVectorNumberOfFields == 3);
    write_barrier(this, storage);
}

vm_oop_t VMVector::AWFYGetStorage(int64_t index) {
    // This is a method that is used by the AWFY tests, it does not handle
    // 1-0 indexing conversion
    int64_t const first = INT_VAL(load_ptr(this->first));
    VMArray* const storage = load_ptr(this->storage);

    if (index > storage->GetNumberOfIndexableFields()) {
        return load_ptr(nilObject);  // AWFY does not handle an IndexOutOfBounds
                                     // in this case
    }
    vm_oop_t returned = storage->GetIndexableField(
        (first - 1) + (index - 1));  // Convert to 0-indexing
    return returned;
}

vm_oop_t VMVector::GetStorage(int64_t index) {
    int64_t const first = INT_VAL(load_ptr(this->first));
    int64_t const last = INT_VAL(load_ptr(this->last));
    VMArray* const storage = load_ptr(this->storage);

    if (index < 1 || index > last - first) {
        return IndexOutOfBounds(first + last - 1, index);
    }
    vm_oop_t returned = storage->GetIndexableField(
        (first - 1) + (index - 1));  // Convert to 0-indexing
    return returned;
}

/* Returns the value currently held at that location */
vm_oop_t VMVector::SetStorage(int64_t index, vm_oop_t value) {
    int64_t const first = INT_VAL(load_ptr(this->first));
    int64_t const last = INT_VAL(load_ptr(this->last));
    VMArray* const storage = load_ptr(this->storage);
    if (index < 1 || index > first + last) {
        return IndexOutOfBounds(first + last - 1, index);
    }
    vm_oop_t curVal = storage->GetIndexableField(first + index - 2);
    storage->SetIndexableField(first + index - 2, value);
    return curVal;
}

/* AWFY Vector can expand on at:put: core-lib vector cannot*/
void VMVector::SetStorageAWFY(int64_t index, vm_oop_t value) {
    int64_t const first = INT_VAL(load_ptr(this->first));
    int64_t last = INT_VAL(load_ptr(this->last));
    VMArray* storage = load_ptr(this->storage);

    if (index > storage->GetNumberOfIndexableFields()) {
        // Expand the array
        VMArray* newStorage = Universe::NewExpandedArrayFromArray(
            storage->GetNumberOfIndexableFields() * 2, storage);

        newStorage->SetIndexableField(index - 1, value);

        this->storage = store_ptr(this->storage, newStorage);

    } else {
        // Just set the new value
        storage->SetIndexableField(first + index - 2, value);
        last += 1;
        this->last = store_ptr(this->last, NEW_INT(last));
    }
}

void VMVector::Append(vm_oop_t value) {
    int64_t last = INT_VAL(load_ptr(this->last));
    VMArray* storage = load_ptr(this->storage);

    if (last >= storage->GetNumberOfIndexableFields()) {  // Expand the array
        // Expand by the correct amount *2 by default in the native som
        // implementation
        VMArray* newStorage = Universe::NewExpandedArrayFromArray(
            storage->GetNumberOfIndexableFields() * 2, storage);

        storage->CopyIndexableFieldsTo(newStorage);
        newStorage->SetIndexableField(last - 1, value);

        SetField(2 /* storage */, newStorage);

    } else {  // Just set the new value
        storage->SetIndexableField(last - 1, value);
    }

    last += 1;
    this->last = store_ptr(this->last, NEW_INT(last));
}

vm_oop_t VMVector::RemoveLast() {
    const int64_t last = INT_VAL(load_ptr(this->last));
    const int64_t first = INT_VAL(load_ptr(this->first));

    // Throw an error correctly (error: method in som)
    if (last == first) {
        // VMSafe*Primitive::Invoke will push it right back to the same frame
        VMFrame* frame = Interpreter::GetFrame();
        vm_oop_t errorMsg =
            Universe::NewString("Vector: error when removing Last item.");
        vm_oop_t args[1] = {errorMsg};
        this->Send("error:", args, 1);
        return frame->Pop();
    }
    return Remove(NEW_INT(last - first));  // Last-First gives the (User) index
                                           // of the last element in 1-indexing
}

vm_oop_t VMVector::RemoveFirst() {
    // This method will just increment the first index
    int64_t first = INT_VAL(load_ptr(this->first));

    // Throw an error correctly (error: method in som)
    if (first >= INT_VAL(load_ptr(this->last))) {
        // VMSafe*Primitive::Invoke will push it right back to the same frame
        VMFrame* frame = Interpreter::GetFrame();
        vm_oop_t errorMsg =
            Universe::NewString("Vector: error when removing First item.");
        vm_oop_t args[1] = {errorMsg};
        this->Send("error:", args, 1);
        return frame->Pop();
    }
    vm_oop_t itemToRemove = GetStorage(
        1);      // This is 1 because GetIndexableField handles 1 to 0 indexing
    first += 1;  // Increment the first index
    this->first = store_ptr(this->first, NEW_INT(first));
    return itemToRemove;
}

vm_oop_t VMVector::RemoveObj(vm_oop_t other) {
    const int64_t first = INT_VAL(load_ptr(this->first));
    const int64_t last = INT_VAL(load_ptr(this->last));
    VMArray* storage = load_ptr(this->storage);

    for (int64_t i = first - 1; i < last - 1; ++i) {
        vm_oop_t current = storage->GetIndexableField(i);

        // Check where integers are tagged or references can be checked
        if (current == other) {
            Remove(NEW_INT(i - first + 2));  // Convert to 1-indexing
            return load_ptr(trueObject);
        }
    }
    return load_ptr(falseObject);
}

vm_oop_t VMVector::Remove(vm_oop_t inx) {
    const int64_t first = INT_VAL(load_ptr(this->first));
    int64_t last = INT_VAL(load_ptr(this->last));
    VMArray* storage = load_ptr(this->storage);
    int64_t const index = INT_VAL(inx);

    if (index < 1 || index > last - first) {
        return IndexOutOfBounds(first + last - 1, index);
    }

    vm_oop_t itemToRemove = GetStorage(index);

    // Shift all elements after the index to the left
    for (int64_t i = index; i < last - first; ++i) {
        storage->SetIndexableField(first + i - 1,
                                   storage->GetIndexableField(first + i));
    }

    last -= 1;
    this->last = store_ptr(this->last, NEW_INT(last));

    return itemToRemove;
}

void VMVector::RemoveAll() {
    this->first = store_ptr(this->first, NEW_INT(1));
    this->last = store_ptr(this->last, NEW_INT(1));
    VMArray* storage = load_ptr(this->storage);
    VMArray* newArray =
        Universe::NewArray(storage->GetNumberOfIndexableFields());
    this->storage = store_ptr(this->storage, newArray);
}

vm_oop_t VMVector::copyStorageArray() {
    const int64_t first = INT_VAL(load_ptr(this->first));
    const int64_t last = INT_VAL(load_ptr(this->last));
    VMArray* storage = load_ptr(this->storage);

    VMArray* result = Universe::NewArray(last - first);
    for (int64_t i = first - 1; i < last - 1; ++i) {
        result->SetIndexableField(i - first + 1, storage->GetIndexableField(i));
    }

    return result;
}

/* Rename as a more specific error function */
vm_oop_t VMVector::IndexOutOfBounds(size_t maxSize, size_t indexAccessed) {
    // VMSafe*Primitive::Invoke will push it right back to the same frame
    VMFrame* frame = Interpreter::GetFrame();
    vm_oop_t errorMsg = Universe::NewString(
        "Vector[1.." + std::to_string(maxSize) + "]: Index " +
        std::to_string(indexAccessed) + " out of bounds");
    vm_oop_t args[1] = {errorMsg};
    this->Send("error:", args, 1);
    return frame->Pop();
}

vm_oop_t VMVector::IndexOutOfBoundsAWFY() {
    VMFrame* frame = Interpreter::GetFrame();
    this->Send("println", nullptr, 1);
    return frame->Pop();
}
