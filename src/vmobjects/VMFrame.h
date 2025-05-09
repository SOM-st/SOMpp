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

#include <cstdint>
#include <type_traits>

#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMMethod.h"

class Universe;

class VMFrame : public AbstractVMObject {
    friend class Universe;
    friend class Interpreter;
    friend class Shell;
    friend class VMMethod;

public:
    typedef GCFrame Stored;

    static VMFrame* EmergencyFrameFrom(VMFrame* from, size_t extraLength);

    explicit VMFrame(size_t additionalBytes, VMMethod* method,
                     VMFrame* previousFrame)
        : totalObjectSize(additionalBytes + sizeof(VMFrame)),
          previousFrame(store_root(previousFrame)), method(store_root(method)),
          arguments((gc_oop_t*)&(stack_ptr) + 1),
          locals(arguments + method->GetNumberOfArguments()),
          stack_ptr(locals + method->GetNumberOfLocals() - 1) {
        // initilize all other fields. Don't need to initalize arguments,
        // because they iwll be copied in still
        // --> until end of Frame
        auto* end = (gc_oop_t*)SHIFTED_PTR(this, totalObjectSize);
        size_t i = 0;
        while (locals + i < end) {
            locals[i] = nilObject;
            i++;
        }
    }

    [[nodiscard]] int64_t GetHash() const final {
        return 0; /* should never be called */
    }

    [[nodiscard]] inline VMClass* GetClass() const final { return nullptr; }

    [[nodiscard]] inline size_t GetObjectSize() const final {
        return totalObjectSize;
    }

    void MarkObjectAsInvalid() final {
        previousFrame = (GCFrame*)INVALID_GC_POINTER;
    }

    [[nodiscard]] bool IsMarkedInvalid() const final {
        return previousFrame == (GCFrame*)INVALID_GC_POINTER;
    }

    [[nodiscard]] inline VMFrame* GetPreviousFrame() const;
    inline void ClearPreviousFrame();
    [[nodiscard]] inline bool HasPreviousFrame() const;
    [[nodiscard]] inline bool IsBootstrapFrame() const;
    [[nodiscard]] inline VMFrame* GetContext() const;
    inline void SetContext(VMFrame* /*frm*/);
    [[nodiscard]] inline bool HasContext() const;
    VMFrame* GetContextLevel(uint8_t lvl);
    VMFrame* GetOuterContext();
    [[nodiscard]] inline VMMethod* GetMethod() const;

    inline vm_oop_t Pop() {
        vm_oop_t result = load_ptr(*stack_ptr);
        stack_ptr--;
        return result;
    }

    inline void PopVoid() { stack_ptr--; }

    inline vm_oop_t Top() {
        vm_oop_t result = load_ptr(*stack_ptr);
        return result;
    }

    inline vm_oop_t Top2() {
        vm_oop_t result = load_ptr(*(stack_ptr - 1));
        return result;
    }

    inline void SetTop(gc_oop_t val) { *stack_ptr = val; }

    inline void Push(vm_oop_t obj) {
        assert(RemainingStackSize() > 0);
        ++stack_ptr;
        store_ptr(*stack_ptr, obj);
    }

    [[nodiscard]] inline size_t GetBytecodeIndex() const {
        return bytecodeIndex;
    }

    [[nodiscard]] inline vm_oop_t GetStackElement(size_t index) const {
        return load_ptr(stack_ptr[-std::make_signed_t<int64_t>(index)]);
    }

    inline vm_oop_t GetLocal(uint8_t index, uint8_t contextLevel) {
        VMFrame* context = GetContextLevel(contextLevel);
        return load_ptr(context->locals[index]);
    }

    inline vm_oop_t GetLocalInCurrentContext(uint8_t localIndex) {
        return load_ptr(this->locals[localIndex]);
    }

    void SetLocal(uint8_t index, uint8_t contextLevel, vm_oop_t value);

    inline void SetLocal(uint8_t index, vm_oop_t value) {
        store_ptr(locals[index], value);
    }

    inline vm_oop_t GetArgument(uint8_t index, uint8_t contextLevel) {
        // get the context
        VMFrame* context = GetContextLevel(contextLevel);
        return load_ptr(context->arguments[index]);
    }

    inline vm_oop_t GetArgumentInCurrentContext(uint8_t index) {
        return load_ptr(this->arguments[index]);
    }

    void SetArgument(uint8_t index, uint8_t contextLevel, vm_oop_t value);
    void PrintStackTrace() const;
    void CopyArgumentsFrom(VMFrame* frame);

    inline void SetArgument(size_t argIdx, vm_oop_t value) {
        store_ptr(arguments[argIdx], value);
    }

    void WalkObjects(walk_heap_fn /*unused*/) override;
    [[nodiscard]] VMFrame* CloneForMovingGC() const override;

    void PrintStack() const;
    void PrintBytecode() const;

    [[nodiscard]] size_t RemainingStackSize() const {
        // - 1 because the stack pointer points at the top entry,
        // so the next entry would be put at stackPointer+1
        size_t const size =
            ((size_t)this + totalObjectSize - size_t(stack_ptr)) /
            sizeof(VMObject*);
        return size - 1;
    }

    [[nodiscard]] std::string AsDebugString() const override;

    make_testable(public);

    size_t bytecodeIndex{0};
    size_t totalObjectSize;

    void ResetStackPointer() {
        VMMethod* meth = GetMethod();
        // Set the stack pointer to its initial value thereby clearing the stack
        stack_ptr = locals + meth->GetNumberOfLocals() - 1;
    }

    void ResetBytecodeIndex();

private:
    GCFrame* previousFrame;
    GCFrame* context{nullptr};
    GCMethod* method;
    gc_oop_t* arguments;
    gc_oop_t* locals;
    gc_oop_t* stack_ptr;

    static const size_t VMFrameNumberOfFields;

    inline void SetBytecodeIndex(size_t index) { bytecodeIndex = index; }
};

bool VMFrame::HasContext() const {
    return context != nullptr;
}

bool VMFrame::HasPreviousFrame() const {
    return previousFrame != nullptr;
}

bool VMFrame::IsBootstrapFrame() const {
    return !HasPreviousFrame();
}

VMFrame* VMFrame::GetContext() const {
    return load_ptr(context);
}

void VMFrame::SetContext(VMFrame* frm) {
    store_ptr(context, frm);
}

VMFrame* VMFrame::GetPreviousFrame() const {
    return load_ptr(previousFrame);
}

void VMFrame::ClearPreviousFrame() {
    previousFrame = nullptr;
}

VMMethod* VMFrame::GetMethod() const {
    return load_ptr(method);
}
