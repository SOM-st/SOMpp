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

#include "VMInvokable.h"
#include "VMSymbol.h"
#include "VMClass.h"

bool VMInvokable::IsPrimitive() const {
    return false;
}

VMSymbol* VMInvokable::GetSignature() const {
    return signature;
}

void VMInvokable::SetSignature(VMSymbol* sig) {
    signature = sig;
    write_barrier(this, sig);
}

    clazz = static_cast<VMClass*>(walk(clazz));
    signature = static_cast<VMSymbol*>(walk(signature));
void VMInvokable::WalkObjects(walk_heap_fn walk) {
    if (holder)
        holder = static_cast<VMClass*>(walk(holder));
}

VMClass* VMInvokable::GetHolder() const {
    return holder;
}

void VMInvokable::SetHolder(VMClass* hld) {
    holder = hld;
    write_barrier(this, hld);
}
