#include "Primitive.h"

#include <cstddef>

#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMClass.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMSymbol.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete

static vm_oop_t pHolder(vm_oop_t rcvr) {
    auto* self = static_cast<VMInvokable*>(rcvr);
    return self->GetHolder();
}

static vm_oop_t pSignature(vm_oop_t rcvr) {
    auto* self = static_cast<VMInvokable*>(rcvr);
    return self->GetSignature();
}

static void pInvokeOnWith(VMFrame* frame) {
    // REM: this is a clone with _Method mInvokeOnWith
    auto* args = static_cast<VMArray*>(frame->Pop());
    auto* rcvr = frame->Pop();
    auto* mthd = static_cast<VMInvokable*>(frame->Pop());

    frame->Push(rcvr);

    size_t const num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }
    mthd->Invoke(frame);
}

_Primitive::_Primitive() {
    Add("signature", &pSignature, false);
    Add("holder", &pHolder, false);
    Add("invokeOn:with:", &pInvokeOnWith, false);
}
