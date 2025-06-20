#include "VMSafePrimitive.h"

#include <string>

#include "../memory/Heap.h"
#include "../misc/debug.h"
#include "../misc/defs.h"
#include "../primitivesCore/Primitives.h"
#include "../vm/Print.h"
#include "AbstractObject.h"
#include "ObjectFormats.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMMethod.h"
#include "VMSymbol.h"

VMInvokable* VMSafePrimitive::GetSafeUnary(VMSymbol* sig, UnaryPrim prim) {
    auto* p = new (GetHeap<HEAP_CLS>(), 0) VMSafeUnaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeUnaryPrimitive::Invoke(VMFrame* frame) {
    vm_oop_t receiverObj = frame->Pop();

    frame->Push(prim.pointer(receiverObj));
    return nullptr;
}

VMFrame* VMSafeUnaryPrimitive::Invoke1(VMFrame* frame) {
    vm_oop_t receiverObj = frame->Pop();

    frame->Push(prim.pointer(receiverObj));
    return nullptr;
}

VMInvokable* VMSafePrimitive::GetSafeBinary(VMSymbol* sig, BinaryPrim prim) {
    auto* p = new (GetHeap<HEAP_CLS>(), 0) VMSafeBinaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeBinaryPrimitive::Invoke(VMFrame* frame) {
    vm_oop_t rightObj = frame->Pop();
    vm_oop_t leftObj = frame->Pop();

    frame->Push(prim.pointer(leftObj, rightObj));
    return nullptr;
}

VMFrame* VMSafeBinaryPrimitive::Invoke1(VMFrame* /*frame*/) {
    ErrorExit("Unary invoke on binary primitive");
}

VMInvokable* VMSafePrimitive::GetSafeTernary(VMSymbol* sig, TernaryPrim prim) {
    auto* p = new (GetHeap<HEAP_CLS>(), 0) VMSafeTernaryPrimitive(sig, prim);
    return p;
}

VMFrame* VMSafeTernaryPrimitive::Invoke(VMFrame* frame) {
    vm_oop_t arg2 = frame->Pop();
    vm_oop_t arg1 = frame->Pop();
    vm_oop_t self = frame->Pop();

    frame->Push(prim.pointer(self, arg1, arg2));
    return nullptr;
}

VMFrame* VMSafeTernaryPrimitive::Invoke1(VMFrame* /*frame*/) {
    ErrorExit("Unary invoke on binary primitive");
}

std::string VMSafePrimitive::AsDebugString() const {
    return "SafePrim(" + GetClass()->GetName()->GetStdString() + ">>#" +
           GetSignature()->GetStdString() + ")";
}

AbstractVMObject* VMSafeUnaryPrimitive::CloneForMovingGC() const {
    auto* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeUnaryPrimitive(*this);
    return prim;
}

AbstractVMObject* VMSafeBinaryPrimitive::CloneForMovingGC() const {
    auto* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeBinaryPrimitive(*this);
    return prim;
}

AbstractVMObject* VMSafeTernaryPrimitive::CloneForMovingGC() const {
    auto* prim =
        new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMSafeTernaryPrimitive(*this);
    return prim;
}

void VMSafePrimitive::InlineInto(MethodGenerationContext& /*mgenc*/,
                                 bool /*mergeScope*/) {
    ErrorExit(
        "VMPrimitive::InlineInto is not supported, and should not be reached");
}

void VMSafePrimitive::Dump(const char* /*indent*/, bool /*printObjects*/) {
    DebugPrint("<primitive>\n");
}
