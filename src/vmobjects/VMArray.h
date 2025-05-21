#pragma once

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
#include <cstddef>

#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMObject.h"

/**
 * For the VMArray, we assume that there are no subclasses, and that `Array`
 * doesn't have any fields itself. This way, we just used the fields as
 * indexable fields.
 */
class VMArray : public VMObject {
public:
    typedef GCArray Stored;

    explicit VMArray(size_t arraySize, size_t additionalBytes)
        : VMObject(arraySize +
                       0 /* VMArray is not allowed to have any fields itself */,
                   additionalBytes + sizeof(VMArray)) {
        assert(VMArrayNumberOfFields == 0);
    }

    explicit VMArray(size_t arraySize,
                     size_t additionalBytes,
                     size_t nillableFrom)
        : VMObject(arraySize +
                       0 /* VMArray is not allowed to have any fields itself */,
                   additionalBytes + sizeof(VMArray),
                   nillableFrom) {
        assert(VMArrayNumberOfFields == 0);
    }

    // VMArray doesn't need to customize `void WalkObjects(walk_heap_fn)`,
    // because it doesn't need anything special.

    [[nodiscard]] inline size_t GetNumberOfIndexableFields() const {
        return numberOfFields;
    }

    /** Used from language level, via primitive */
    [[nodiscard]] VMArray* Copy() const;

    VMArray* CopyAndExtendWith(vm_oop_t /*item*/) const;

    [[nodiscard]] inline vm_oop_t GetIndexableField(size_t idx) const {
        if (unlikely(idx > numberOfFields)) {
            IndexOutOfBounds(idx);
        }
        return GetField(idx);
    }

    inline void SetIndexableField(size_t idx, vm_oop_t value) {
        if (unlikely(idx > GetNumberOfIndexableFields())) {
            IndexOutOfBounds(idx);
        }
        SetField(idx, value);
    }

    __attribute__((noreturn)) __attribute__((noinline)) void IndexOutOfBounds(
        size_t idx) const;

    void CopyIndexableFieldsTo(VMArray* /*to*/) const;
    [[nodiscard]] VMArray* CloneForMovingGC() const override;

    [[nodiscard]] std::string AsDebugString() const override;

private:
    static const size_t VMArrayNumberOfFields;
};
