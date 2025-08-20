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

#include "VMObject.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Universe.h"
#include "ObjectFormats.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMSymbol.h"

// clazz is the only field of VMObject so
const size_t VMObject::VMObjectNumberOfFields = 0;

VMObject* VMObject::CloneForMovingGC() const {
    auto* clone =
        new (GetHeap<HEAP_CLS>(),
             totalObjectSize - sizeof(VMObject) ALLOC_MATURE) VMObject(*this);
    memcpy(SHIFTED_PTR(clone, sizeof(VMObject)),
           SHIFTED_PTR(this, sizeof(VMObject)),
           totalObjectSize - sizeof(VMObject));
    return clone;
}

void VMObject::nilInitializeFields() {
    for (size_t i = 0; i < numberOfFields; ++i) {
        FIELDS[i] = nilObject;
    }
}

/* Allows only a certain subset of fields to be made nil */
void VMObject::nilInitializeFieldsFrom(size_t nillableFrom) {
    for (size_t i = nillableFrom; i < numberOfFields; ++i) {
        FIELDS[i] = nilObject;
    }
}

void VMObject::SetClass(VMClass* cl) {
    store_ptr(clazz, cl);
}

VMSymbol* VMObject::GetFieldName(size_t index) const {
    return load_ptr(clazz)->GetInstanceFieldName(index);
}

void VMObject::Assert(bool value) const {
    Universe::Assert(value);
}

void VMObject::WalkObjects(walk_heap_fn walk) {
    clazz = static_cast<GCClass*>(walk(clazz));

    size_t const numFields = GetNumberOfFields();
    for (size_t i = 0; i < numFields; ++i) {
        FIELDS[i] = walk(tmp_ptr(GetField(i)));
    }
}

void VMObject::MarkObjectAsInvalid() {
    clazz = (GCClass*)INVALID_GC_POINTER;

    size_t const numFields = GetNumberOfFields();
    for (size_t i = 0; i < numFields; ++i) {
        FIELDS[i] = INVALID_GC_POINTER;
    }
}

bool VMObject::IsMarkedInvalid() const {
    return clazz == INVALID_GC_POINTER;
}

std::string VMObject::AsDebugString() const {
    if (this == load_ptr(nilObject)) {
        return "nilObject";
    }
    if (this == load_ptr(trueObject)) {
        return "trueObject";
    }
    if (this == load_ptr(falseObject)) {
        return "falseObject";
    }
    return "Object(" + GetClass()->GetName()->GetStdString() + ")";
}
