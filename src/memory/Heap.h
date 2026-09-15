#pragma once

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

#include <cstdlib>
#include <set>
#include <vector>

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "GarbageCollector.h"

/*
 * macro for padding - only word-aligned memory must be allocated
 */
#define PADDED_SIZE(N) \
    (((((size_t)(N)) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1)))

#define IS_PADDED_SIZE(N) ((N) == PADDED_SIZE((N)))

using namespace std;

/*
 * If true, request a GC after every single allocation.
 */
#if DEBUG
extern bool gcStressMode;
#else
  #define gcStressMode false
#endif

template <class HEAP_T>
class Heap {
    friend class GarbageCollector<HEAP_T>;
    friend HEAP_T;

private:
    explicit Heap(GarbageCollector<HEAP_T>* const gc) : gc(gc) {}

public:
    static void InitializeHeap(size_t objectSpaceSize);
    static void DestroyHeap();

    ~Heap();
    inline void requestGC() { gcWasRequested = true; }
    inline void resetGCTrigger() { gcWasRequested = false; }
    bool isCollectionTriggered() { return gcWasRequested; }
    void FullGC();
    inline void FreeObject(AbstractVMObject* o) { free(o); }

protected:
    GarbageCollector<HEAP_T>* const gc;

private:
    template <class HEAP_U>
    friend HEAP_U* GetHeap();
    static HEAP_T* theHeap;

    // flag that shows if a Collection is triggered
    bool gcWasRequested{false};
};

template <class HEAP_T>
HEAP_T* Heap<HEAP_T>::theHeap;

template <class HEAP_T>
inline HEAP_T* GetHeap() __attribute__((always_inline));
template <class HEAP_T>
HEAP_T* GetHeap() {
    return Heap<HEAP_T>::theHeap;
}
