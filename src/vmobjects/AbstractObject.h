#pragma once

/*
 * AbstractVMObject.h
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include <cassert>
#include <iostream>

#include "../memory/CopyingHeap.h"
#include "../memory/DebugCopyingHeap.h"
#include "../memory/DebugMarkSweepHeap.h"
#include "../memory/GenerationalHeap.h"
#include "../memory/MarkSweepHeap.h"
#include "../misc/defs.h"
#include "../vm/Print.h"
#include "ObjectFormats.h"
#include "VMObjectBase.h"

using namespace std;

// this is the base class for all VMObjects
class AbstractVMObject : public VMObjectBase {
public:
    typedef GCAbstractObject Stored;

    [[nodiscard]] virtual int64_t GetHash() const = 0;
    [[nodiscard]] virtual VMClass* GetClass() const = 0;
    [[nodiscard]] virtual AbstractVMObject* CloneForMovingGC() const = 0;
    void Send(const std::string& selectorString, vm_oop_t* arguments,
              size_t argc);

    /** Size in bytes of the object. */
    [[nodiscard]] virtual size_t GetObjectSize() const = 0;

    virtual void MarkObjectAsInvalid() = 0;
    [[nodiscard]] virtual bool IsMarkedInvalid() const = 0;

    [[nodiscard]] virtual std::string AsDebugString() const = 0;

    AbstractVMObject() { gcfield = 0; }
    ~AbstractVMObject() override = default;

    [[nodiscard]] inline virtual size_t GetNumberOfFields() const {
        ErrorPrint("this object doesn't support GetNumberOfFields\n");
        return -1;
    }

    inline virtual void SetClass(VMClass* /*unused*/) {
        ErrorPrint("this object doesn't support SetClass\n");
    }

    int64_t GetFieldIndex(VMSymbol* fieldName) const;

    virtual void WalkObjects(walk_heap_fn /*walk*/) {}

    [[nodiscard]] inline virtual VMSymbol* GetFieldName(
        size_t /*index*/) const {
        ErrorPrint("this object doesn't support GetFieldName\n");
        return nullptr;
    }

    /**
     * usage: new( <heap> , <additionalBytes>) VMObject( <constructor params> )
     * num_bytes parameter is set by the compiler.
     * parameter additional_bytes (a_b) is used for:
     *   - fields in VMObject, a_b must be set to
     * (numberOfFields*sizeof(VMObject*))
     *   - chars in VMString/VMSymbol, a_b must be set to (Stringlength + 1)
     *   - array size in VMArray; a_b must be set to
     * (size_of_array*sizeof(VMObect*))
     *   - fields in VMMethod, a_b must be set to (number_of_bc +
     * number_of_csts*sizeof(VMObject*))
     */
    void* operator new(size_t numBytes, HEAP_CLS* heap,
                       size_t additionalBytes ALLOC_OUTSIDE_NURSERY_DECL) {
        // if outsideNursery flag is set or object is too big for nursery, we
        // allocate a mature object
        assert(IS_PADDED_SIZE(additionalBytes));

        void* result = nullptr;
#if GC_TYPE == GENERATIONAL
        if (outsideNursery) {
            result =
                (void*)heap->AllocateMatureObject(numBytes + additionalBytes);
        } else {
            result =
                (void*)heap->AllocateNurseryObject(numBytes + additionalBytes);
        }
#else
        result = (void*)heap->AllocateObject(numBytes + additionalBytes);
#endif

        assert(result != INVALID_VM_POINTER);
        return result;
    }
};
