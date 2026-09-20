#include "VMTrivialMethod.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "../compiler/BytecodeGenerator.h"
#include "../compiler/MethodGenerationContext.h"
#include "../compiler/Variable.h"
#include "../interpreter/Interpreter.h"
#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../misc/defs.h"
#include "../vm/LogAllocation.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "AbstractObject.h"
#include "ObjectFormats.h"
#include "VMFrame.h"

VMTrivialMethod* MakeLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                                   vm_oop_t literal) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), 0) VMLiteralReturn(sig, arguments, literal);
    LOG_ALLOCATION("VMLiteralReturn", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeGlobalReturn(VMSymbol* sig, vector<Variable>& arguments,
                                  VMSymbol* globalName) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), 0) VMGlobalReturn(sig, arguments, globalName);
    LOG_ALLOCATION("VMGlobalReturn", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeGetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), 0) VMGetter(sig, arguments, fieldIndex);
    LOG_ALLOCATION("VMGetter", result->GetObjectSize());
    return result;
}

VMTrivialMethod* MakeSetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex, size_t argIndex) {
    auto* result = new (GetHeap<HEAP_CLS>(), 0)
        VMSetter(sig, arguments, fieldIndex, argIndex);
    LOG_ALLOCATION("VMSetter", result->GetObjectSize());
    return result;
}

VMFrame* VMLiteralReturn::Invoke(VMFrame* frame) {
    for (int i = 0; i < numberOfArguments; i += 1) {
        frame->Pop();
    }
    frame->Push(load_ptr(literal));
    return nullptr;
}

VMFrame* VMLiteralReturn::Invoke1(VMFrame* frame) {
    assert(numberOfArguments == 1);
    frame->Pop();
    frame->Push(load_ptr(literal));
    return nullptr;
}

AbstractVMObject* VMLiteralReturn::CloneForMovingGC() const {
    auto* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMLiteralReturn(*this);
    return prim;
}

std::string VMLiteralReturn::AsDebugString() const {
    return "VMLiteralReturn(" + AS_OBJ(load_ptr(literal))->AsDebugString() +
           ")";
}

void VMLiteralReturn::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
    literal = walk(literal);
}

void VMLiteralReturn::InlineInto(MethodGenerationContext& mgenc,
                                 const Parser& parser, bool /*mergeScope*/) {
    EmitPUSHCONSTANT(mgenc, parser, load_ptr(literal));
}

VMFrame* VMGlobalReturn::Invoke(VMFrame* frame) {
    for (int i = 0; i < numberOfArguments; i += 1) {
        frame->Pop();
    }

    vm_oop_t value = Universe::GetGlobal(load_ptr(globalName));
    if (value != nullptr) {
        frame->Push(value);
    } else {
        Interpreter::SendUnknownGlobal(load_ptr(globalName));
    }

    return nullptr;
}

VMFrame* VMGlobalReturn::Invoke1(VMFrame* frame) {
    assert(numberOfArguments == 1);
    frame->Pop();

    vm_oop_t value = Universe::GetGlobal(load_ptr(globalName));
    if (value != nullptr) {
        frame->Push(value);
    } else {
        Interpreter::SendUnknownGlobal(load_ptr(globalName));
    }

    return nullptr;
}

void VMGlobalReturn::InlineInto(MethodGenerationContext& mgenc,
                                const Parser& parser, bool /*mergeScope*/) {
    EmitPUSHGLOBAL(mgenc, parser, load_ptr(globalName));
}

void VMGlobalReturn::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
    globalName = (GCSymbol*)walk(globalName);
}

std::string VMGlobalReturn::AsDebugString() const {
    return "VMGlobalReturn(" + AS_OBJ(load_ptr(globalName))->AsDebugString() +
           ")";
}

AbstractVMObject* VMGlobalReturn::CloneForMovingGC() const {
    auto* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMGlobalReturn(*this);
    return prim;
}

VMFrame* VMGetter::Invoke(VMFrame* frame) {
    vm_oop_t self = nullptr;
    for (int i = 0; i < numberOfArguments; i += 1) {
        self = frame->Pop();
    }

    assert(self != nullptr);

    vm_oop_t result = nullptr;
    if (unlikely(IS_TAGGED(self))) {
        result = nullptr;
        ErrorExit("Integers do not have fields!");
    } else {
        result = ((VMObject*)self)->GetField(fieldIndex);
    }

    frame->Push(result);

    return nullptr;
}

VMFrame* VMGetter::Invoke1(VMFrame* frame) {
    assert(numberOfArguments == 1);
    vm_oop_t self = frame->Pop();

    assert(self != nullptr);

    vm_oop_t result = nullptr;
    if (unlikely(IS_TAGGED(self))) {
        result = nullptr;
        ErrorExit("Integers do not have fields!");
    } else {
        result = ((VMObject*)self)->GetField(fieldIndex);
    }

    frame->Push(result);

    return nullptr;
}

void VMGetter::InlineInto(MethodGenerationContext& mgenc,
                          const Parser& /*parser*/, bool /*mergeScope*/) {
    EmitPushFieldWithIndex(mgenc, fieldIndex);
}

AbstractVMObject* VMGetter::CloneForMovingGC() const {
    auto* prim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMGetter(*this);
    return prim;
}

void VMGetter::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
}

std::string VMGetter::AsDebugString() const {
    return "VMGetter(fieldIndex: " + to_string(fieldIndex) + ")";
}

VMFrame* VMSetter::Invoke(VMFrame* frame) {
    vm_oop_t value = nullptr;
    vm_oop_t self = nullptr;

    for (size_t i = numberOfArguments - 1; i > 0; i -= 1) {
        if (i == argIndex) {
            value = frame->Pop();
        } else {
            frame->Pop();
        }
    }

    self = frame->Top();
    assert(self != nullptr);
    assert(value != nullptr);

    if (unlikely(IS_TAGGED(self))) {
        ErrorExit("Integers do not have fields!");
    } else {
        ((VMObject*)self)->SetField(fieldIndex, value);
    }

    return nullptr;
}

VMFrame* VMSetter::Invoke1(VMFrame* /*unused*/) {
    ErrorExit("VMSetter::Invoke1 should not be reachable");
}

void VMSetter::InlineInto(MethodGenerationContext& /*mgenc*/,
                          const Parser& /*parser*/, bool /*mergeScope*/) {
    ErrorExit("We don't currently support blocks for trivial setters");
}

AbstractVMObject* VMSetter::CloneForMovingGC() const {
    auto* prim = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSetter(*this);
    return prim;
}

void VMSetter::WalkObjects(walk_heap_fn walk) {
    VMInvokable::WalkObjects(walk);
}

std::string VMSetter::AsDebugString() const {
    return "VMSetter(fieldIndex: " + to_string(fieldIndex) +
           ", argIndex: " + to_string(argIndex) + ")";
}

void VMTrivialMethod::Dump(const char* /*indent*/, bool /*printObjects*/) {
    DebugPrint("<trivial method>\n");
}
