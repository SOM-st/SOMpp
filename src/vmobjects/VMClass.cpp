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

#include "VMClass.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../primitivesCore/PrimitiveLoader.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Print.h"
#include "ObjectFormats.h"
#include "VMArray.h"
#include "VMInvokable.h"
#include "VMMethod.h"
#include "VMObject.h"
#include "VMSymbol.h"

const size_t VMClass::VMClassNumberOfFields = 4;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
VMClass::VMClass() : VMObject(VMClassNumberOfFields, sizeof(VMClass)) {}

VMClass* VMClass::CloneForMovingGC() const {
    auto* clone =
        new (GetHeap<HEAP_CLS>(),
             totalObjectSize - sizeof(VMClass) ALLOC_MATURE) VMClass(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this, sizeof(VMObject)),
           GetObjectSize() - sizeof(VMObject));
    return clone;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
VMClass::VMClass(size_t numberOfFields, size_t additionalBytes)
    : VMObject(numberOfFields + VMClassNumberOfFields,
               additionalBytes + sizeof(VMClass)) {}

PrimInstallResult VMClass::InstallPrimitive(VMInvokable* invokable,
                                            size_t bytecodeHash,
                                            bool reportHashMismatch) {
    if (invokable == nullptr) {
        ErrorExit("Error: trying to add non-invokable to invokables array");
        return PrimInstallResult::NULL_ARG;
    }

    // Check whether an invokable with the same signature exists and replace it
    // if that's the case
    VMArray* instInvokables = load_ptr(instanceInvokables);
    size_t const numIndexableFields =
        instInvokables->GetNumberOfIndexableFields();

    for (size_t i = 0; i < numIndexableFields; ++i) {
        auto* inv =
            static_cast<VMInvokable*>(instInvokables->GetIndexableField(i));

        if (inv == nullptr) {
            ErrorExit(
                "Invokables array corrupted. "
                "Either NULL pointer added or pointer to non-invokable.");
            return PrimInstallResult::NULL_IN_INVOKABLES;
        }

        if (invokable->GetSignature() == inv->GetSignature()) {
            PrimInstallResult result = PrimInstallResult::NULL_ARG;
            bool hashMismatch = false;
            size_t seenHash = 0;

            if (bytecodeHash != 0) {
                auto* method = dynamic_cast<VMMethod*>(inv);
                if (method == nullptr) {
                    assert(inv->GetHolder() != nullptr);
                    ErrorPrint(
                        "Expected a bytecode method, but found something else "
                        "for " +
                        inv->GetHolder()->GetName()->GetStdString() + ">>#" +
                        inv->GetSignature()->GetStdString());
                    result = PrimInstallResult::HASH_MISMATCH;
                    hashMismatch = true;
                } else {
                    seenHash = method->GetBytecodeHash();
                    if (seenHash == bytecodeHash) {
                        result = PrimInstallResult::INSTALLED_REPLACED;
                    } else {
                        result = PrimInstallResult::HASH_MISMATCH;
                        hashMismatch = true;
                    }
                }
            } else {
                result = PrimInstallResult::INSTALLED_REPLACED;
            }

            if (!hashMismatch) {
                SetInstanceInvokable(i, invokable);
            } else if (reportHashMismatch) {
                cout << "Warn: Primitive "
                     << inv->GetHolder()->GetName()->GetStdString() << ">>#"
                     << invokable->GetSignature()->GetStdString()
                     << " was not installed.\n"
                     << "Warn: Bytecode hash did not match.\n"
                     << "Warn: expected hash: " << bytecodeHash << "\n"
                     << "Warn: actual hash:   " << seenHash << "\n";
            }
            return result;
        }
    }
    // it's a new invokable so we need to expand the invokables array.
    store_ptr(instanceInvokables,
              instInvokables->CopyAndExtendWith((vm_oop_t)invokable));

    return PrimInstallResult::INSTALLED_ADDED;
}

VMSymbol* VMClass::GetInstanceFieldName(size_t index) const {
    size_t const numSuperInstanceFields = numberOfSuperInstanceFields();
    if (index >= numSuperInstanceFields) {
        index -= numSuperInstanceFields;
        return static_cast<VMSymbol*>(
            load_ptr(instanceFields)->GetIndexableField(index));
    }
    assert(HasSuperClass());
    return ((VMClass*)load_ptr(superClass))->GetInstanceFieldName(index);
}

void VMClass::SetInstanceInvokables(VMArray* invokables) {
    store_ptr(instanceInvokables, invokables);
    vm_oop_t nil = load_ptr(nilObject);

    size_t const numInvokables = GetNumberOfInstanceInvokables();
    for (size_t i = 0; i < numInvokables; ++i) {
        vm_oop_t invo = load_ptr(instanceInvokables)->GetIndexableField(i);
        // check for Nil object
        if (invo != nil) {
            // not Nil, so this actually is an invokable
            auto* inv = (VMInvokable*)invo;
            inv->SetHolder(this);
        }
    }
}

size_t VMClass::GetNumberOfInstanceInvokables() const {
    return load_ptr(instanceInvokables)->GetNumberOfIndexableFields();
}

VMInvokable* VMClass::GetInstanceInvokable(size_t index) const {
    return static_cast<VMInvokable*>(
        load_ptr(instanceInvokables)->GetIndexableField(index));
}

void VMClass::SetInstanceInvokable(size_t index, VMInvokable* invokable) {
    load_ptr(instanceInvokables)->SetIndexableField(index, invokable);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    if (invokable != reinterpret_cast<VMInvokable*>(load_ptr(nilObject))) {
        invokable->SetHolder(this);
    }
}

VMInvokable* VMClass::LookupInvokable(VMSymbol* name) {
    assert(IsValidObject(this));

    VMInvokable* invokable = name->GetCachedInvokable(this);
    if (invokable != nullptr) {
        return invokable;
    }

    size_t const numInvokables = GetNumberOfInstanceInvokables();
    for (size_t i = 0; i < numInvokables; ++i) {
        invokable = GetInstanceInvokable(i);
        if (invokable->GetSignature() == name) {
            name->UpdateCachedInvokable(this, invokable);
            return invokable;
        }
    }

    // look in super class
    if (HasSuperClass()) {
        return ((VMClass*)load_ptr(superClass))->LookupInvokable(name);
    }

    // invokable not found
    return nullptr;
}

int64_t VMClass::LookupFieldIndex(VMSymbol* name) const {
    size_t const numInstanceFields = GetNumberOfInstanceFields();
    for (size_t i = 0; i <= numInstanceFields; ++i) {
        // even with GetNumberOfInstanceFields == 0 there is the class field
        if (name == GetInstanceFieldName(i)) {
            return (int64_t)i;
        }
    }
    return -1;
}

size_t VMClass::GetNumberOfInstanceFields() const {
    return load_ptr(instanceFields)->GetNumberOfIndexableFields() +
           numberOfSuperInstanceFields();
}

bool VMClass::HasPrimitives() const {
    size_t const numInvokables = GetNumberOfInstanceInvokables();
    for (size_t i = 0; i < numInvokables; ++i) {
        VMInvokable* invokable = GetInstanceInvokable(i);
        if (invokable->IsPrimitive()) {
            return true;
        }
    }
    return false;
}

void VMClass::LoadPrimitives(bool showWarning) {
    std::string const cname = load_ptr(name)->GetStdString();

    if (hasPrimitivesFor(cname)) {
        PrimitiveLoader::InstallPrimitives(cname, this, false, showWarning);
        PrimitiveLoader::InstallPrimitives(
            cname, GetClass(), true, showWarning);
    }
}

size_t VMClass::numberOfSuperInstanceFields() const {
    if (HasSuperClass()) {
        return ((VMClass*)load_ptr(superClass))->GetNumberOfInstanceFields();
    }
    return 0;
}

bool VMClass::hasPrimitivesFor(const std::string& cl) {
    return PrimitiveLoader::SupportsClass(cl);
}

std::string VMClass::AsDebugString() const {
    return "Class(" + GetName()->GetStdString() + ")";
}

VMObject* VMClass::GetSuperClass() const {
    return load_ptr(superClass);
}

void VMClass::SetSuperClass(VMObject* sup) {
    store_ptr(superClass, sup);
}

VMSymbol* VMClass::GetName() const {
    return load_ptr(name);
}

void VMClass::SetName(VMSymbol* nam) {
    store_ptr(name, nam);
}

bool VMClass::HasSuperClass() const {
    assert(IsValidObject(load_ptr(superClass)));
    return load_ptr(superClass) != load_ptr(nilObject);
}

VMArray* VMClass::GetInstanceFields() const {
    return load_ptr(instanceFields);
}

void VMClass::SetInstanceFields(VMArray* instFields) {
    store_ptr(instanceFields, instFields);
}

VMArray* VMClass::GetInstanceInvokables() const {
    return load_ptr(instanceInvokables);
}
