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

#include "Interpreter.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "../compiler/Disassembler.h"
#include "../interpreter/bytecodes.h"  // NOLINT(misc-include-cleaner) it's required for InterpreterLoop.h
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMSymbol.h"

const std::string Interpreter::unknownGlobal = "unknownGlobal:";
const std::string Interpreter::doesNotUnderstand =
    "doesNotUnderstand:arguments:";
const std::string Interpreter::escapedBlock = "escapedBlock:";

VMFrame* Interpreter::frame = nullptr;
VMMethod* Interpreter::method = nullptr;

// The following three variables are used to cache main parts of the
// current execution context
size_t Interpreter::bytecodeIndexGlobal;
uint8_t* Interpreter::currentBytecodes;

vm_oop_t Interpreter::StartAndPrintBytecodes() {
#define PROLOGUE(bcCount)                 \
    {                                     \
        disassembleMethod();              \
        bytecodeIndexGlobal += (bcCount); \
    }
#define HACK_INLINE_START
#include "InterpreterLoop.h"
#undef HACK_INLINE_START
}

vm_oop_t Interpreter::Start() {
#undef PROLOGUE
#define PROLOGUE(bcCount) \
    { bytecodeIndexGlobal += (bcCount); }
#define HACK_INLINE_START
#include "InterpreterLoop.h"
#undef HACK_INLINE_START
}

VMFrame* Interpreter::PushNewFrame(VMMethod* method) {
    SetFrame(Universe::NewFrame(GetFrame(), method));
    return GetFrame();
}

void Interpreter::SetFrame(VMFrame* frm) {
    if (frame != nullptr) {
        frame->SetBytecodeIndex(bytecodeIndexGlobal);
    }

    frame = frm;

    // update cached values
    method = frm->GetMethod();
    bytecodeIndexGlobal = frm->GetBytecodeIndex();
    currentBytecodes = method->GetBytecodes();
}

vm_oop_t Interpreter::GetSelf() {
    VMFrame* context = GetFrame()->GetOuterContext();
    return context->GetArgumentInCurrentContext(0);
}

VMFrame* Interpreter::popFrame() {
    VMFrame* result = GetFrame();
    SetFrame(GetFrame()->GetPreviousFrame());

    result->ClearPreviousFrame();

#ifdef UNSAFE_FRAME_OPTIMIZATION
    // remember this frame as free frame
    result->GetMethod()->SetCachedFrame(result);
#endif
    return result;
}

void Interpreter::popFrameAndPushResult(vm_oop_t result) {
    VMFrame* prevFrame = popFrame();

    VMMethod* method = prevFrame->GetMethod();
    uint8_t const numberOfArgs = method->GetNumberOfArguments();

    for (uint8_t i = 0; i < numberOfArgs; ++i) {
        GetFrame()->Pop();
    }

    GetFrame()->Push(result);
}

void Interpreter::send(VMSymbol* signature, VMClass* receiverClass) {
    VMInvokable* invokable = receiverClass->LookupInvokable(signature);

    if (invokable != nullptr) {
#ifdef LOG_RECEIVER_TYPES
        std::string name = receiverClass->GetName()->GetStdString();
        if (Universe::callStats.find(name) == Universe::callStats.end()) {
            Universe::callStats[name] = {0, 0};
        }
        Universe::callStats[name].noCalls++;
        if (invokable->IsPrimitive()) {
            Universe::callStats[name].noPrimitiveCalls++;
        }
#endif

        invokable->Invoke(GetFrame());
    } else {
        triggerDoesNotUnderstand(signature);
    }
}

void Interpreter::triggerDoesNotUnderstand(VMSymbol* signature) {
    uint8_t const numberOfArgs = Signature::GetNumberOfArguments(signature);

    vm_oop_t receiver = GetFrame()->GetStackElement(numberOfArgs - 1);

    VMArray* argumentsArray =
        Universe::NewArray(numberOfArgs - 1);  // without receiver

    // the receiver should not go into the argumentsArray
    // so, numberOfArgs - 2
    for (int64_t i = numberOfArgs - 2; i >= 0; --i) {
        vm_oop_t o = GetFrame()->Pop();
        argumentsArray->SetIndexableField(i, o);
    }
    vm_oop_t arguments[] = {signature, argumentsArray};

    GetFrame()->Pop();  // pop the receiver

    // check if current frame is big enough for this unplanned Send
    // doesNotUnderstand: needs 3 slots, one for this, one for method name, one
    // for args
    int64_t const additionalStackSlots =
        3 - (int64_t)GetFrame()->RemainingStackSize();
    if (additionalStackSlots > 0) {
        GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
        // copy current frame into a bigger one and replace the current frame
        SetFrame(VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots));
    }

    AS_OBJ(receiver)->Send(doesNotUnderstand, arguments, 2);
}

void Interpreter::doDup() {
    vm_oop_t elem = GetFrame()->GetStackElement(0);
    GetFrame()->Push(elem);
}

void Interpreter::doPushLocal(size_t bytecodeIndex) {
    uint8_t const bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t const bc2 = method->GetBytecode(bytecodeIndex + 2);

    assert((bc1 != 0 || bc2 != 0) && "should have been BC_PUSH_LOCAL_0");
    assert((bc1 != 1 || bc2 != 0) && "should have been BC_PUSH_LOCAL_1");
    assert((bc1 != 2 || bc2 != 0) && "should have been BC_PUSH_LOCAL_2");

    vm_oop_t local = GetFrame()->GetLocal(bc1, bc2);

    GetFrame()->Push(local);
}

void Interpreter::doPushLocalWithIndex(uint8_t localIndex) {
    vm_oop_t local = GetFrame()->GetLocalInCurrentContext(localIndex);
    GetFrame()->Push(local);
}

void Interpreter::doPushArgument(size_t bytecodeIndex) {
    uint8_t const argIndex = method->GetBytecode(bytecodeIndex + 1);
    uint8_t const contextLevel = method->GetBytecode(bytecodeIndex + 2);

    assert((argIndex != 0 || contextLevel != 0) &&
           "should have been BC_PUSH_SELF");
    assert((argIndex != 1 || contextLevel != 0) &&
           "should have been BC_PUSH_ARG_1");
    assert((argIndex != 2 || contextLevel != 0) &&
           "should have been BC_PUSH_ARG_2");

    vm_oop_t argument = GetFrame()->GetArgument(argIndex, contextLevel);

    GetFrame()->Push(argument);
}

void Interpreter::doPushField(size_t bytecodeIndex) {
    uint8_t const fieldIndex = method->GetBytecode(bytecodeIndex + 1);
    assert(fieldIndex != 0 && fieldIndex != 1 &&
           "should have been BC_PUSH_FIELD_0|1");

    doPushFieldWithIndex(fieldIndex);
}

void Interpreter::doPushFieldWithIndex(uint8_t fieldIndex) {
    vm_oop_t self = GetSelf();
    vm_oop_t o = nullptr;

    if (unlikely(IS_TAGGED(self))) {
        o = nullptr;
        ErrorExit("Integers do not have fields!");
    } else {
        o = ((VMObject*)self)->GetField(fieldIndex);
    }

    GetFrame()->Push(o);
}

void Interpreter::doReturnFieldWithIndex(uint8_t fieldIndex) {
    vm_oop_t self = GetSelf();
    vm_oop_t o = nullptr;

    if (unlikely(IS_TAGGED(self))) {
        o = nullptr;
        ErrorExit("Integers do not have fields!");
    } else {
        o = ((VMObject*)self)->GetField(fieldIndex);
    }

    popFrameAndPushResult(o);
}

void Interpreter::doPushBlock(size_t bytecodeIndex) {
    vm_oop_t block = method->GetConstant(bytecodeIndex);
    auto* blockMethod = static_cast<VMInvokable*>(block);

    uint8_t const numOfArgs = blockMethod->GetNumberOfArguments();

    GetFrame()->Push(Universe::NewBlock(blockMethod, GetFrame(), numOfArgs));
}

void Interpreter::doPushGlobal(size_t bytecodeIndex) {
    auto* globalName =
        static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));
    vm_oop_t global = Universe::GetGlobal(globalName);

    if (global != nullptr) {
        GetFrame()->Push(global);
    } else {
        SendUnknownGlobal(globalName);
    }
}

void Interpreter::SendUnknownGlobal(VMSymbol* globalName) {
    vm_oop_t arguments[] = {globalName};
    vm_oop_t self = GetSelf();

    // check if there is enough space on the stack for this unplanned Send
    // unknowGlobal: needs 2 slots, one for "this" and one for the argument
    int64_t const additionalStackSlots =
        2 - (int64_t)GetFrame()->RemainingStackSize();
    if (additionalStackSlots > 0) {
        GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
        // copy current frame into a bigger one and replace the current
        // frame
        SetFrame(VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots));
    }

    AS_OBJ(self)->Send(unknownGlobal, arguments, 1);
}

void Interpreter::doPop() {
    GetFrame()->Pop();
}

void Interpreter::doPopLocal(size_t bytecodeIndex) {
    uint8_t const bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t const bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t o = GetFrame()->Pop();

    GetFrame()->SetLocal(bc1, bc2, o);
}

void Interpreter::doPopLocalWithIndex(uint8_t localIndex) {
    vm_oop_t o = GetFrame()->Pop();
    GetFrame()->SetLocal(localIndex, o);
}

void Interpreter::doPopArgument(size_t bytecodeIndex) {
    uint8_t const bc1 = method->GetBytecode(bytecodeIndex + 1);
    uint8_t const bc2 = method->GetBytecode(bytecodeIndex + 2);

    vm_oop_t o = GetFrame()->Pop();
    GetFrame()->SetArgument(bc1, bc2, o);
}

void Interpreter::doPopField(size_t bytecodeIndex) {
    uint8_t const fieldIndex = method->GetBytecode(bytecodeIndex + 1);
    doPopFieldWithIndex(fieldIndex);
}

void Interpreter::doPopFieldWithIndex(uint8_t fieldIndex) {
    vm_oop_t self = GetSelf();
    vm_oop_t o = GetFrame()->Pop();

    if (unlikely(IS_TAGGED(self))) {
        ErrorExit("Integers do not have fields that can be set");
    } else {
        ((VMObject*)self)->SetField(fieldIndex, o);
    }
}

void Interpreter::doSend(size_t bytecodeIndex) {
    auto* signature =
        static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

    uint8_t const numOfArgs = Signature::GetNumberOfArguments(signature);

    vm_oop_t receiver = GetFrame()->GetStackElement(numOfArgs - 1);

    assert(IsValidObject(receiver));
    // make sure it is really a class
    assert(dynamic_cast<VMClass*>(CLASS_OF(receiver)) != nullptr);

    VMClass* receiverClass = CLASS_OF(receiver);

    assert(IsValidObject(receiverClass));

#ifdef LOG_RECEIVER_TYPES
    Universe::receiverTypes[receiverClass->GetName()->GetStdString()]++;
#endif

    send(signature, receiverClass);
}

void Interpreter::doUnarySend(size_t bytecodeIndex) {
    auto* signature =
        static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

    const int numOfArgs = 1;

    vm_oop_t receiver = frame->GetStackElement(numOfArgs - 1);

    assert(IsValidObject(receiver));
    // make sure it is really a class
    assert(dynamic_cast<VMClass*>(CLASS_OF(receiver)) != nullptr);

    VMClass* receiverClass = CLASS_OF(receiver);

    assert(IsValidObject(receiverClass));

#ifdef LOG_RECEIVER_TYPES
    Universe::receiverTypes[receiverClass->GetName()->GetStdString()]++;
#endif

    VMInvokable* invokable = receiverClass->LookupInvokable(signature);

    if (invokable != nullptr) {
#ifdef LOG_RECEIVER_TYPES
        std::string name = receiverClass->GetName()->GetStdString();
        if (Universe::callStats.find(name) == Universe::callStats.end()) {
            Universe::callStats[name] = {0, 0};
        }
        Universe::callStats[name].noCalls++;
        if (invokable->IsPrimitive()) {
            Universe::callStats[name].noPrimitiveCalls++;
        }
#endif
        invokable->Invoke1(GetFrame());
    } else {
        triggerDoesNotUnderstand(signature);
    }
}

void Interpreter::doSuperSend(size_t bytecodeIndex) {
    auto* signature =
        static_cast<VMSymbol*>(method->GetConstant(bytecodeIndex));

    VMFrame* ctxt = GetFrame()->GetOuterContext();
    VMMethod* realMethod = ctxt->GetMethod();
    VMClass* holder = realMethod->GetHolder();
    assert(holder->HasSuperClass());
    auto* super = (VMClass*)holder->GetSuperClass();
    auto* invokable = super->LookupInvokable(signature);

    if (invokable != nullptr) {
        invokable->Invoke(GetFrame());
    } else {
        uint8_t const numOfArgs = Signature::GetNumberOfArguments(signature);
        vm_oop_t receiver = GetFrame()->GetStackElement(numOfArgs - 1);
        VMArray* argumentsArray = Universe::NewArray(numOfArgs);

        for (uint8_t i = numOfArgs - 1; i >= 0; --i) {
            vm_oop_t o = GetFrame()->Pop();
            argumentsArray->SetIndexableField(i, o);
        }
        vm_oop_t arguments[] = {signature, argumentsArray};

        AS_OBJ(receiver)->Send(doesNotUnderstand, arguments, 2);
    }
}

void Interpreter::doReturnLocal() {
    vm_oop_t result = GetFrame()->Pop();
    popFrameAndPushResult(result);
}

void Interpreter::doReturnNonLocal() {
    vm_oop_t result = GetFrame()->Pop();

    VMFrame* context = GetFrame()->GetOuterContext();

    if (!context->HasPreviousFrame()) {
        auto* block =
            static_cast<VMBlock*>(GetFrame()->GetArgumentInCurrentContext(0));
        VMFrame* prevFrame = GetFrame()->GetPreviousFrame();
        VMFrame* outerContext = prevFrame->GetOuterContext();
        vm_oop_t sender = outerContext->GetArgumentInCurrentContext(0);
        vm_oop_t arguments[] = {block};

        popFrame();

        // Pop old arguments from stack
        VMMethod* method = GetFrame()->GetMethod();
        uint8_t const numberOfArgs = method->GetNumberOfArguments();
        for (uint8_t i = 0; i < numberOfArgs; ++i) {
            GetFrame()->Pop();
        }

        // check if current frame is big enough for this unplanned send
        // #escapedBlock: needs 2 slots, one for self, and one for the block
        int64_t const additionalStackSlots =
            2 - (int64_t)GetFrame()->RemainingStackSize();
        if (additionalStackSlots > 0) {
            GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
            // copy current frame into a bigger one, and replace it
            SetFrame(
                VMFrame::EmergencyFrameFrom(GetFrame(), additionalStackSlots));
        }

        AS_OBJ(sender)->Send(escapedBlock, arguments, 1);
        return;
    }

    while (GetFrame() != context) {
        popFrame();
    }

    popFrameAndPushResult(result);
}

void Interpreter::doInc() {
    vm_oop_t val = GetFrame()->Top();

    if (IS_TAGGED(val) || CLASS_OF(val) == load_ptr(integerClass)) {
        int64_t const result = (int64_t)INT_VAL(val) + 1;
        val = NEW_INT(result);
    } else if (CLASS_OF(val) == load_ptr(doubleClass)) {
        double const d = static_cast<VMDouble*>(val)->GetEmbeddedDouble();
        val = Universe::NewDouble(d + 1.0);
    } else {
        ErrorExit("unsupported");
    }

    GetFrame()->SetTop(store_root(val));
}

void Interpreter::doDec() {
    vm_oop_t val = GetFrame()->Top();

    if (IS_TAGGED(val) || CLASS_OF(val) == load_ptr(integerClass)) {
        int64_t const result = (int64_t)INT_VAL(val) - 1;
        val = NEW_INT(result);
    } else if (CLASS_OF(val) == load_ptr(doubleClass)) {
        double const d = static_cast<VMDouble*>(val)->GetEmbeddedDouble();
        val = Universe::NewDouble(d - 1.0);
    } else {
        ErrorExit("unsupported");
    }

    GetFrame()->SetTop(store_root(val));
}

void Interpreter::doIncField(uint8_t fieldIndex) {
    vm_oop_t self = GetSelf();

    if (unlikely(IS_TAGGED(self))) {
        ErrorExit("Integers do not have fields!");
    }

    auto* selfObj = (VMObject*)self;

    vm_oop_t val = selfObj->GetField(fieldIndex);

    if (IS_TAGGED(val) || CLASS_OF(val) == load_ptr(integerClass)) {
        int64_t const result = (int64_t)INT_VAL(val) + 1;
        val = NEW_INT(result);
    } else if (CLASS_OF(val) == load_ptr(doubleClass)) {
        double const d = static_cast<VMDouble*>(val)->GetEmbeddedDouble();
        val = Universe::NewDouble(d + 1.0);
    } else {
        ErrorExit("unsupported");
    }

    selfObj->SetField(fieldIndex, val);
}

void Interpreter::doIncFieldPush(uint8_t fieldIndex) {
    vm_oop_t self = GetSelf();

    if (unlikely(IS_TAGGED(self))) {
        ErrorExit("Integers do not have fields!");
    }

    auto* selfObj = (VMObject*)self;

    vm_oop_t val = selfObj->GetField(fieldIndex);

    if (IS_TAGGED(val) || CLASS_OF(val) == load_ptr(integerClass)) {
        int64_t const result = (int64_t)INT_VAL(val) + 1;
        val = NEW_INT(result);
    } else if (CLASS_OF(val) == load_ptr(doubleClass)) {
        double const d = static_cast<VMDouble*>(val)->GetEmbeddedDouble();
        val = Universe::NewDouble(d + 1.0);
    } else {
        ErrorExit("unsupported");
    }

    selfObj->SetField(fieldIndex, val);
    GetFrame()->Push(val);
}

bool Interpreter::checkIsGreater() {
    vm_oop_t top = GetFrame()->Top();
    vm_oop_t top2 = GetFrame()->Top2();

    if ((IS_TAGGED(top) || CLASS_OF(top) == load_ptr(integerClass)) &&
        (IS_TAGGED(top2) || CLASS_OF(top2) == load_ptr(integerClass))) {
        return INT_VAL(top) > INT_VAL(top2);
    }
    if ((CLASS_OF(top) == load_ptr(doubleClass)) &&
        (CLASS_OF(top2) == load_ptr(doubleClass))) {
        return static_cast<VMDouble*>(top)->GetEmbeddedDouble() >
               static_cast<VMDouble*>(top2)->GetEmbeddedDouble();
    }

    return false;
}

void Interpreter::WalkGlobals(walk_heap_fn walk) {
    method = load_ptr(static_cast<GCMethod*>(walk(tmp_ptr(method))));

    // Get the current frame and mark it.
    // Since marking is done recursively, this automatically
    // marks the whole stack
    frame = load_ptr(static_cast<GCFrame*>(walk(tmp_ptr(frame))));
}

void Interpreter::startGC() {
    GetFrame()->SetBytecodeIndex(bytecodeIndexGlobal);
    GetHeap<HEAP_CLS>()->FullGC();
    method = GetFrame()->GetMethod();
    currentBytecodes = method->GetBytecodes();
}

VMMethod* Interpreter::GetMethod() {
    return GetFrame()->GetMethod();
}

uint8_t* Interpreter::GetBytecodes() {
    return method->GetBytecodes();
}

void Interpreter::disassembleMethod() {
    Disassembler::DumpBytecode(GetFrame(), GetMethod(), bytecodeIndexGlobal);
}
