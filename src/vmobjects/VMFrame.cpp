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

#include "VMFrame.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "../compiler/Disassembler.h"
#include "../interpreter/Interpreter.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "ObjectFormats.h"
#include "VMClass.h"
#include "VMMethod.h"
#include "VMObject.h"
#include "VMObjectBase.h"  // NOLINT(misc-include-cleaner) needed for some GCs
#include "VMSymbol.h"

// when doesNotUnderstand or UnknownGlobal is sent, additional stack slots might
// be necessary, as these cases are not taken into account when the stack
// depth is calculated. In that case this method is called.
VMFrame* VMFrame::EmergencyFrameFrom(VMFrame* from, size_t extraLength) {
    VMMethod* method = from->GetMethod();
    size_t const length =
        method->GetNumberOfArguments() + method->GetNumberOfLocals() +
        method->GetMaximumNumberOfStackElements() + extraLength;

    size_t const additionalBytes = length * sizeof(VMObject*);
    auto* result = new (GetHeap<HEAP_CLS>(), additionalBytes)
        VMFrame(additionalBytes, method, from->GetPreviousFrame());

    // set Frame members
    result->SetContext(from->GetContext());
    result->stack_ptr =
        (gc_oop_t*)SHIFTED_PTR(result, (size_t)from->stack_ptr - (size_t)from);

    result->bytecodeIndex = from->bytecodeIndex;
    // result->arguments is set in VMFrame constructor
    result->locals = result->arguments + method->GetNumberOfArguments();

    // all other fields are indexable via arguments
    // --> until end of Frame
    auto* from_end = (gc_oop_t*)SHIFTED_PTR(from, from->GetObjectSize());
    auto* result_end = (gc_oop_t*)SHIFTED_PTR(result, result->GetObjectSize());

    size_t i = 0;

    // copy all fields from other frame
    while (from->arguments + i < from_end) {
        result->arguments[i] = from->arguments[i];
        i++;
    }
    // initialize others with nilObject
    while (result->arguments + i < result_end) {
        result->arguments[i] = nilObject;
        i++;
    }
    return result;
}

VMFrame* VMFrame::CloneForMovingGC() const {
    size_t const addSpace = totalObjectSize - sizeof(VMFrame);
    auto* clone =
        new (GetHeap<HEAP_CLS>(), addSpace ALLOC_MATURE) VMFrame(*this);
    void* destination = SHIFTED_PTR(clone, sizeof(VMFrame));
    const void* source = SHIFTED_PTR(this, sizeof(VMFrame));
    memcpy(destination, source, addSpace);
    clone->arguments =
        (gc_oop_t*)&(clone->stack_ptr) + 1;  // field after stack_ptr

    // Use of GetMethod() is problematic here, because it may be invalid object
    // while cloning/moving within GC Use of GetMethod()->GetNumberOfArguments()
    // is problematic here, because it may be invalid object while
    // cloning/moving within GC

#if GC_TYPE == GENERATIONAL || GC_TYPE == COPYING || GC_TYPE == DEBUG_COPYING
    VMMethod* meth = load_ptr(method);
    if (meth->GetGCField() != 0 && meth->GetGCField() != MASK_OBJECT_IS_OLD) {
        meth = (VMMethod*)meth->GetGCField();
    }
//    int64_t numArgs =
//    meth->GetNumberOfArgumentsPossiblyFollowingForwardingPointer();
#else
    VMMethod* meth = GetMethod();
#endif
    uint8_t const numArgs = meth->GetNumberOfArguments();

    clone->locals = clone->arguments + numArgs;
    clone->stack_ptr =
        (gc_oop_t*)SHIFTED_PTR(clone, (size_t)stack_ptr - (size_t)this);
    return clone;
}

const size_t VMFrame::VMFrameNumberOfFields = 0;

VMFrame* VMFrame::GetContextLevel(uint8_t lvl) {
    VMFrame* current = this;
    while (lvl > 0) {
        current = current->GetContext();
        --lvl;
    }
    return current;
}

VMFrame* VMFrame::GetOuterContext() {
    VMFrame* current = this;
    while (current->HasContext()) {
        current = current->GetContext();
    }
    return current;
}

void VMFrame::WalkObjects(walk_heap_fn walk) {
    // VMFrame is not a proper SOM object any longer, we don't have a class for
    // it. clazz = (VMClass*) walk(clazz);

    if (previousFrame != nullptr) {
        previousFrame = static_cast<GCFrame*>(walk(previousFrame));
    }
    if (context != nullptr) {
        context = static_cast<GCFrame*>(walk(context));
    }
    method = static_cast<GCMethod*>(walk(method));

    // all other fields are indexable via arguments array
    // --> until end of Frame
    size_t i = 0;
    while (arguments + i <= stack_ptr) {
        if (arguments[i] != nullptr) {
            arguments[i] = walk(arguments[i]);
        }
        i++;
    }
}

void VMFrame::PrintBytecode() const {
    Disassembler::DumpMethod(GetMethod(), "  ");
}

static void print_oop(gc_oop_t vmo) {
    if (vmo == nullptr) {
        Print("nullptr\n");
    } else if (vmo == nilObject) {
        Print("NIL_OBJECT\n");
    } else {
        AbstractVMObject* o = AS_OBJ(vmo);
        Print(o->AsDebugString() + "\n");
    }
}

void VMFrame::PrintStack() const {
    Print(GetMethod()->AsDebugString() +
          ", bc: " + to_string(GetBytecodeIndex()) + "\n" +
          "Args: " + to_string(GetMethod()->GetNumberOfArguments()) +
          " Locals: " + to_string(GetMethod()->GetNumberOfLocals()) +
          " MaxStack:" +
          to_string(GetMethod()->GetMaximumNumberOfStackElements()) + "\n");

    for (uint8_t i = 0; i < GetMethod()->GetNumberOfArguments(); i++) {
        Print("   arg " + to_string(i) + ": ");
        print_oop(arguments[i]);
    }

    size_t local_offset = 0;
    for (size_t i = 0; i < GetMethod()->GetNumberOfLocals(); i++) {
        Print("   loc " + to_string(i) + ": ");
        print_oop(locals[i]);
        local_offset++;
    }

    size_t const max = GetMethod()->GetMaximumNumberOfStackElements();
    for (size_t i = 0; i < max; i++) {
        if (stack_ptr == &locals[local_offset + i]) {
            Print("-> stk " + to_string(i) + ": ");
        } else {
            Print("   stk " + to_string(i) + ": ");
        }
        print_oop(locals[local_offset + i]);
    }

    auto* end = (gc_oop_t*)SHIFTED_PTR(this, totalObjectSize);
    size_t i = 0;
    while (&locals[local_offset + max + i] < end) {
        if (stack_ptr == &locals[local_offset + max + i]) {
            Print("->estk " + to_string(i) + ": ");
        } else {
            Print("  estk " + to_string(i) + ": ");
        }
        print_oop(locals[local_offset + max + i]);
        i++;
    }
}

void VMFrame::SetLocal(uint8_t index, uint8_t contextLevel, vm_oop_t value) {
    VMFrame* context = GetContextLevel(contextLevel);
    context->SetLocal(index, value);
}

void VMFrame::SetArgument(uint8_t index, uint8_t contextLevel, vm_oop_t value) {
    VMFrame* context = GetContextLevel(contextLevel);
    context->SetArgument(index, value);
}

void VMFrame::PrintStackTrace() const {
    VMMethod* meth = GetMethod();

    if (meth->GetHolder() == load_ptr(nilObject)) {
        Print("nil");
    } else {
        Print(meth->GetHolder()->GetName()->GetStdString());
    }
    Print(">>#" + meth->GetSignature()->GetStdString() + "\n");
    if (previousFrame != nullptr) {
        load_ptr(previousFrame)->PrintStackTrace();
    }
}

void VMFrame::CopyArgumentsFrom(VMFrame* frame) {
    // copy arguments from frame:
    // - arguments are at the top of the stack of frame.
    // - copy them into the argument area of the current frame
    size_t const num_args = GetMethod()->GetNumberOfArguments();
    for (size_t i = 0; i < num_args; ++i) {
        vm_oop_t stackElem = frame->GetStackElement(num_args - 1 - i);
        store_ptr(arguments[i], stackElem);
    }
}

std::string VMFrame::AsDebugString() const {
    return "VMFrame(" + GetMethod()->AsDebugString() + ")";
}

void VMFrame::ResetBytecodeIndex() {
    bytecodeIndex = 0;
    Interpreter::ResetBytecodeIndex(this);
}
