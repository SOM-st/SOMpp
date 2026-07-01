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

#include "Heap.h"

#include <cstring>

#include "../misc/defs.h"
#include "../vm/Print.h"
#include "CopyingHeap.h"         // NOLINT(misc-include-cleaner)
#include "DebugCopyingHeap.h"    // NOLINT(misc-include-cleaner)
#include "DebugMarkSweepHeap.h"  // NOLINT(misc-include-cleaner)
#include "GenerationalHeap.h"    // NOLINT(misc-include-cleaner)
#include "MarkSweepHeap.h"       // NOLINT(misc-include-cleaner)

template <class HEAP_T>
void Heap<HEAP_T>::InitializeHeap(size_t objectSpaceSize) {
    if (theHeap) {
        ErrorPrint(
            "Warning, reinitializing already initialized Heap, "
            "all data will be lost!\n");
        delete theHeap;
    }
    theHeap = new HEAP_CLS(objectSpaceSize);
}

template <class HEAP_T>
void Heap<HEAP_T>::DestroyHeap() {
    delete theHeap;
}

template <class HEAP_T>
Heap<HEAP_T>::~Heap() {
    delete gc;
}

template <class HEAP_T>
void Heap<HEAP_T>::FullGC() {
    gc->Collect();
}

// Instantitate Template for the heap classes
template void Heap<HEAP_CLS>::InitializeHeap(size_t);
template void Heap<HEAP_CLS>::DestroyHeap();
template void Heap<HEAP_CLS>::FullGC();

class GenerationalHeap;
template GenerationalHeap* Heap<GenerationalHeap>::theHeap;
template Heap<GenerationalHeap>::~Heap();

class CopyingHeap;
template CopyingHeap* Heap<CopyingHeap>::theHeap;
template Heap<CopyingHeap>::~Heap();

class MarkSweepHeap;
template MarkSweepHeap* Heap<MarkSweepHeap>::theHeap;
template Heap<MarkSweepHeap>::~Heap();

class DebugMarkSweepHeap;
template DebugMarkSweepHeap* Heap<DebugMarkSweepHeap>::theHeap;
template Heap<DebugMarkSweepHeap>::~Heap();
