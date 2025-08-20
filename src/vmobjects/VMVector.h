#pragma once

#include <cstddef>

#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMBigInteger.h"
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
    [[nodiscard]] vm_oop_t GetStorage(int64_t index);
    [[nodiscard]] vm_oop_t AWFYGetStorage(int64_t index);

    /* Return the first element */
    [[nodiscard]] inline vm_oop_t GetFirst() {
        vm_oop_t returned = GetStorage(1);
        return returned;
    }

    /* Return the last element */
    [[nodiscard]] inline vm_oop_t GetLast() {
        const int64_t last = SMALL_INT_VAL(load_ptr(this->last));
        vm_oop_t returned = GetStorage(last - 1);
        return returned;
    }

    /* Setter Methods */

    /* handles 1 - 0 indexing give the SOM index to this function */
    vm_oop_t SetStorage(int64_t index, vm_oop_t value);
    void SetStorageAWFY(int64_t index, vm_oop_t value);

    /* Append an item to end of Vector */
    void Append(vm_oop_t value);

    /* Remove Methods */

    vm_oop_t RemoveLast();

    vm_oop_t RemoveFirst();

    vm_oop_t RemoveObj(vm_oop_t other);

    vm_oop_t Remove(vm_oop_t inx);

    void RemoveAll();

    /* General Utility Methods */

    /* Size of the Vector */
    [[nodiscard]] inline vm_oop_t Size() {
        const int64_t first = SMALL_INT_VAL(load_ptr(this->first));
        const int64_t last = SMALL_INT_VAL(load_ptr(this->last));
        return NEW_INT(last - first);
    }

    [[nodiscard]] inline vm_oop_t Capacity() {
        VMArray* storage = load_ptr(this->storage);
        return NEW_INT(storage->GetNumberOfIndexableFields());
    }

    /* Return the underlying array */
    [[nodiscard]] vm_oop_t copyStorageArray();

    vm_oop_t IndexOutOfBounds(size_t maxSize, size_t indexAccessed);
    vm_oop_t IndexOutOfBoundsAWFY();

private:
    static const size_t VMVectorNumberOfFields;

    gc_oop_t first;
    gc_oop_t last;
    GCArray* storage;
};
