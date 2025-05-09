#include "Method.h"

#include <cstddef>

#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMClass.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMSymbol.h"  // NOLINT(misc-include-cleaner) it's required to make the types complete

static vm_oop_t mHolder(vm_oop_t rcvr) {
    auto* self = static_cast<VMMethod*>(rcvr);
    return self->GetHolder();
}

static vm_oop_t mSignature(vm_oop_t rcvr) {
    auto* self = static_cast<VMMethod*>(rcvr);
    return self->GetSignature();
}

static void mInvokeOnWith(VMFrame* frame) {
    // REM: this is a clone with _Primitive pInvokeOnWith
    auto* args = static_cast<VMArray*>(frame->Pop());
    auto* rcvr = frame->Pop();
    auto* mthd = static_cast<VMMethod*>(frame->Pop());

    frame->Push(rcvr);

    size_t const num_args = args->GetNumberOfIndexableFields();
    for (size_t i = 0; i < num_args; i++) {
        vm_oop_t arg = args->GetIndexableField(i);
        frame->Push(arg);
    }
    mthd->Invoke(frame);
}

_Method::_Method() {
    Add("signature", &mSignature, false);
    Add("holder", &mHolder, false);
    Add("invokeOn:with:", &mInvokeOnWith, false);
}
