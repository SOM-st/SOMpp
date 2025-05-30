#pragma once

#include <cstddef>

#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMObject.h"
#include "ObjectFormats.h"
#include "VMArray.h"

class VMVector : public VMObject {
public:
    typedef GCVector Stored;

    explicit VMVector(vm_oop_t first, vm_oop_t last, VMArray* storage);

    /* Getter Methods */

    /* handles 1 - 0 indexing give the SOM index to this function */
    [[nodiscard]] vm_oop_t GetIndexableField(size_t index);

    /* Return the first element */
    [[nodiscard]] inline vm_oop_t GetFirst() {
        vm_oop_t returned = GetIndexableField(1);
        return returned;
    }

    /* Return the last element */
    [[nodiscard]] inline vm_oop_t GetLast() {
        const int64_t last = INT_VAL(load_ptr(this->last));
        vm_oop_t returned = GetIndexableField(last - 1);
        return returned;
    }

    /* Setter Methods */

    /* handles 1 - 0 indexing give the SOM index to this function */
    void SetIndexableField(size_t index, vm_oop_t value);

    /* Append an item to end of Vector */
    void Append(vm_oop_t value);

    /* Remove Methods */

    vm_oop_t RemoveLast();

    vm_oop_t RemoveFirst();

    vm_oop_t RemoveObj(vm_oop_t other);

    vm_oop_t Remove(vm_oop_t inx);

    /* General Utility Methods */

    /* Return the index if object is located, else return -1 for not found*/
    [[nodiscard]] vm_oop_t IndexOf(vm_oop_t other);

    /* True if object located, false otherwise */
    [[nodiscard]] inline vm_oop_t Contains(vm_oop_t other) {
        vm_oop_t where = VMVector::IndexOf(other);
        if (INT_VAL(where) < 0) {
            return load_ptr(falseObject);
        }

        return load_ptr(trueObject);
    }

    /* Size of the Vector */
    [[nodiscard]] inline vm_oop_t Size() {
        const int64_t first = INT_VAL(load_ptr(this->first));
        const int64_t last = INT_VAL(load_ptr(this->last));
        return NEW_INT(last - first);
    }

    /* Return the underlying array */
    [[nodiscard]] vm_oop_t StorageArray();

    static __attribute__((noreturn)) __attribute__((noinline)) void
    IndexOutOfBounds(size_t idx, size_t size);

private:
    static const size_t VMVectorNumberOfFields;

    gc_oop_t first;
    gc_oop_t last;
    GCArray* storage;
};
