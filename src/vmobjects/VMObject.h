#pragma once

#include <assert.h>

/*
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
#include <cstring>
#include <iostream>
#include <vector>

#include "../misc/defs.h"
#include "../vm/IsValidObject.h"
#include "AbstractObject.h"
#include "ObjectFormats.h"

// this macro returns a shifted ptr by offset bytes
#define SHIFTED_PTR(ptr, offset) ((void*)((size_t)(ptr) + (size_t)(offset)))

/* chbol: this table is not correct anymore because of introduction of
 * class AbstractVMObject
 **************************VMOBJECT****************************
 * ____________________________________________________________ *
 *| vtable*          |   0x00 - 0x03                           |*
 *|__________________|_________________________________________|*
 *| hash             |   0x04 - 0x07                           |*
 *| totalObjectSize  |   0x08 - 0x0b                           |*
 *| numberOfFields   |   0x0c - 0x0f                           |*
 *| gcField          |   0x10 - 0x13 (because of alignment)    |*
 *| clazz            |   0x14 - 0x17 [0 indexed instance field]|*
 *|__________________|___0x18__________________________________|*
 *                                                              *
 ****************************************************************
 */

// FIELDS starts indexing after the clazz field
#define FIELDS (((gc_oop_t*)&clazz) + 1)

class VMObject : public AbstractVMObject {
public:
    typedef GCObject Stored;

    /**
     * numberOfFields - including
     */
    explicit VMObject(size_t numSubclassFields, size_t totalObjectSize)
        : totalObjectSize(totalObjectSize),
          numberOfFields(VMObjectNumberOfFields + numSubclassFields) {
        assert(IS_PADDED_SIZE(totalObjectSize));
        assert(totalObjectSize >= sizeof(VMObject));

        // this line would be needed if the VMObject** is used instead of the
        // macro: FIELDS = (VMObject**)&clazz;
        hash = (intptr_t)this;

        nilInitializeFields();
    }

    /* Constructor to be used when making fields nil is only required from a
     * certain index onwards */
    explicit VMObject(size_t numSubclassFields, size_t totalObjectSize,
                      size_t nillableFrom)
        : totalObjectSize(totalObjectSize),
          numberOfFields(VMObjectNumberOfFields + numSubclassFields) {
        assert(IS_PADDED_SIZE(totalObjectSize));
        assert(totalObjectSize >= sizeof(VMObject));

        // this line would be needed if the VMObject** is used instead of the
        // macro: FIELDS = (VMObject**)&clazz;
        hash = (intptr_t)this;

        nilInitializeFieldsFrom(nillableFrom);
    }

    ~VMObject() override = default;

    [[nodiscard]] int64_t GetHash() const override { return hash; }

    [[nodiscard]] inline VMClass* GetClass() const override {
        assert(IsValidObject((VMObject*)load_ptr(clazz)));
        return load_ptr(clazz);
    }

    void SetClass(VMClass* cl) override;
    [[nodiscard]] VMSymbol* GetFieldName(size_t index) const override;

    [[nodiscard]] inline size_t GetNumberOfFields() const override {
        return numberOfFields;
    }

    [[nodiscard]] inline vm_oop_t GetField(size_t index) const {
        assert(numberOfFields > index);
        vm_oop_t result = load_ptr(FIELDS[index]);
        assert(IsValidObject(result));
        return result;
    }

    inline void SetField(size_t index, vm_oop_t value) {
        assert(IsValidObject(value));
        store_ptr(FIELDS[index], value);
    }

    virtual void Assert(bool value) const;
    void WalkObjects(walk_heap_fn walk) override;
    [[nodiscard]] VMObject* CloneForMovingGC() const override;

    /** The total size of the object on the heap. */
    [[nodiscard]] inline size_t GetObjectSize() const override {
        return totalObjectSize;
    }

    void MarkObjectAsInvalid() override;
    [[nodiscard]] bool IsMarkedInvalid() const final;

    [[nodiscard]] std::string AsDebugString() const override;

protected:
    void nilInitializeFields();
    void nilInitializeFieldsFrom(size_t nillableFrom);

    // VMObject essentials
    int64_t hash;

    make_testable(public);

    /** Size of the object in the heap, */
    size_t totalObjectSize;

    /** Number of fields, excluding `class` but including fields of any
     * subclass.
     */
    size_t numberOfFields;

    GCClass* clazz{nullptr};

    // Start of fields. All members beyond after clazz are indexable.
    // clazz has index -1.

private:
    static const size_t VMObjectNumberOfFields;
};
