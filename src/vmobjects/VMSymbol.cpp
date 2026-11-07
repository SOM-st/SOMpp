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

#include "VMSymbol.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"  // NOLINT (misc-include-cleaner)
#include "ObjectFormats.h"
#include "Signature.h"
#include "VMClass.h"
#include "VMInvokable.h"
#include "VMString.h"

VMSymbol::VMSymbol(const size_t length, const char* const str)
    :  // set the chars-pointer to point at the position of the first character
      VMString((char*)((intptr_t)&cachedInvokable + sizeof(cachedInvokable)),
               length),
      numberOfArgumentsOfSignature(
          Signature::DetermineNumberOfArguments(str, length)) {
    size_t i = 0;
    for (; i < length; ++i) {
        chars[i] = str[i];
    }
}

size_t VMSymbol::GetObjectSize() const {
    size_t const size = sizeof(VMSymbol) + PADDED_SIZE(length);
    return size;
}

VMSymbol* VMSymbol::CloneForMovingGC() const {
    auto* result = new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length) ALLOC_MATURE)
        VMSymbol(length, chars);
    return result;
}

VMClass* VMSymbol::GetClass() const {
    return load_ptr(symbolClass);
}

void VMSymbol::WalkObjects(walk_heap_fn walk) {
    // nextCachePos wraps around
    // so, will need to look at all of them
    for (size_t i = 0; i < CacheSize; i++) {
        if (cachedClass_invokable[i] == nullptr) {
            continue;
        }
        walk_ptr(cachedClass_invokable[i], walk);
        walk_ptr(cachedInvokable[i], walk);
    }
}

std::string VMSymbol::AsDebugString() const {
    return "Symbol(" + GetStdString() + ")";
}

void VMSymbol::UpdateCachedInvokable(VMClass* cls, VMInvokable* invo) {
    store_ptr(cachedInvokable[nextCachePos], invo);
    store_ptr(cachedClass_invokable[nextCachePos], cls);
    nextCachePos = (nextCachePos + 1) % CacheSize;
}
