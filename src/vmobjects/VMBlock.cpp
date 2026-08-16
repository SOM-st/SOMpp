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

#include "VMBlock.h"

#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "ObjectFormats.h"
#include "VMEvaluationPrimitive.h"
#include "VMObject.h"

const int VMBlock::VMBlockNumberOfFields = 2;

VMBlock::VMBlock(VMInvokable* method, VMFrame* context)
    : VMObject(VMBlockNumberOfFields, sizeof(VMBlock)),
      blockMethod(store_with_separate_barrier(method)),
      context(store_with_separate_barrier(context)) {
    write_barrier(this, method);
    write_barrier(this, context);
#ifdef DEBUG
    if (context != nullptr) {
        context->captured = true;
    }
#endif
}

VMBlock* VMBlock::CloneForMovingGC() const {
    VMBlock* clone = nullptr;
    clone = new (GetHeap<HEAP_CLS>(), 0 ALLOC_MATURE) VMBlock(*this);
    return clone;
}

VMInvokable* VMBlock::GetMethod() const {
    return load_ptr(blockMethod);
}

VMEvaluationPrimitive* VMBlock::GetEvaluationPrimitive(int numberOfArguments) {
    return new (GetHeap<HEAP_CLS>(), 0)
        VMEvaluationPrimitive(numberOfArguments);
}

std::string VMBlock::AsDebugString() const {
    return "Block(" + load_ptr(blockMethod)->AsDebugString() + ")";
}
