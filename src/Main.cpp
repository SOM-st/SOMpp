/*
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

#include <fstream>
#include <iostream>

#include "compiler/ClassGenerationContext.h"
#include "memory/Heap.h"
#include "misc/defs.h"
#include "vm/Universe.h"
#include "vmobjects/ObjectFormats.h"
#include "vmobjects/VMArray.h"
#include "vmobjects/VMMethod.h"
#include "vmobjects/VMObject.h"
#include "vmobjects/VMString.h"

int32_t main(int32_t argc, char** argv) {
    cout << "This is SOM++" << endl;

    if (GC_TYPE == GENERATIONAL) {
        cout << "\tgarbage collector: generational" << endl;
    } else if (GC_TYPE == COPYING) {
        cout << "\tgarbage collector: copying" << endl;
    } else if (GC_TYPE == MARK_SWEEP) {
        cout << "\tgarbage collector: mark-sweep" << endl;
    } else if (GC_TYPE == DEBUG_COPYING) {
        cout << "\tgarbage collector: debug copying" << endl;
    } else {
        cout << "\tgarbage collector: unknown" << endl;
    }

    if (USE_TAGGING) {
        cout << "\twith tagged integers" << endl;
    } else {
        cout << "\tnot tagging integers" << endl;
    }

    if (CACHE_INTEGER) {
        cout << "\tcaching integers from " << INT_CACHE_MIN_VALUE << " to "
             << INT_CACHE_MAX_VALUE << endl;
    } else {
        cout << "\tnot caching integers" << endl;
    }

    if (USE_VECTOR_PRIMITIVES) {
        cout << "\tVector primitives: enabled" << endl;
    } else {
        cout << "\tVector primitives: disabled" << endl;
    }

    cout << "--------------------------------------" << endl;

    Universe::Start(argc, argv);

    Quit(ERR_SUCCESS);
}
