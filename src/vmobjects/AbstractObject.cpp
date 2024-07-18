/*
 * AbstractVMObject.cpp
 *
 *  Created on: 10.03.2011
 *      Author: christian
 */

#include <cstdint>
#include <string>

#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "AbstractObject.h"
#include "VMClass.h"
#include "VMFrame.h"
#include "VMInvokable.h"
#include "VMSymbol.h"

int64_t AbstractVMObject::GetHash() {
    return (int64_t) this;
}

void AbstractVMObject::Send(Interpreter* interp, std::string selectorString, vm_oop_t* arguments, long argc) {
    VMFrame* frame = interp->GetFrame();
    VMSymbol* selector = GetUniverse()->SymbolFor(selectorString);
    frame->Push(this);

    for (long i = 0; i < argc; ++i) {
        frame->Push(arguments[i]);
    }

    VMClass* cl = GetClass();
    VMInvokable* invokable = cl->LookupInvokable(selector);
    invokable->Invoke(interp, frame);
}

long AbstractVMObject::GetFieldIndex(VMSymbol* fieldName) const {
    return GetClass()->LookupFieldIndex(fieldName);
}
