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

#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMMethod.h"
#include "MethodGenerationContext.h"

class Disassembler {
public:
    static void Dump(VMClass* cl);
    static void DumpMethod(VMMethod* method, const char* indent,
                           bool printObjects = true);
    static void DumpMethod(MethodGenerationContext* mgenc, const char* indent);
    static void extracted(uint8_t bc1, uint8_t bc2, VMClass* cl,
                          VMFrame* frame);

    static void DumpBytecode(VMFrame* frame, VMMethod* method, size_t bc_idx);

private:
    static void dispatch(vm_oop_t o);

    static void dumpMethod(uint8_t* bytecodes, size_t numberOfBytecodes,
                           const char* indent, VMMethod* method,
                           bool printObjects);

    static void printArgument(uint8_t idx, uint8_t ctx, VMClass const* cl,
                              VMFrame* frame);
    static void printPopLocal(uint8_t idx, uint8_t ctx, VMFrame* frame);
    static void printNth(uint8_t idx, VMFrame* frame, const char* op);
    static void printConstantAccess(vm_oop_t constant, uint8_t index);
};
