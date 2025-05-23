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

#include <assert.h>

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMMethod.h"

#define DISPATCH_NOGC()                                           \
    {                                                             \
        goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
    }

#define DISPATCH_GC()                                             \
    {                                                             \
        if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {       \
            startGC();                                            \
        }                                                         \
        goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
    }

class Interpreter {
public:
    template <bool PrintBytecodes>
    static vm_oop_t Start();

    static VMFrame* PushNewFrame(VMMethod* method);
    static void SetFrame(VMFrame* frm);

    static inline VMFrame* GetFrame() { return frame; }

    static VMMethod* GetMethod();
    static uint8_t* GetBytecodes();
    static void WalkGlobals(walk_heap_fn /*walk*/);

    static void SendUnknownGlobal(VMSymbol* globalName);

    static inline size_t GetBytecodeIndex() { return bytecodeIndexGlobal; }

    static void ResetBytecodeIndex(VMFrame* forFrame) {
        assert(frame == forFrame);
        assert(forFrame != nullptr);
        bytecodeIndexGlobal = 0;
    }

private:
    static vm_oop_t GetSelf();

    static VMFrame* frame;
    static VMMethod* method;

    // The following three variables are used to cache main parts of the
    // current execution context
    static size_t bytecodeIndexGlobal;
    static uint8_t* currentBytecodes;

    static const std::string unknownGlobal;
    static const std::string doesNotUnderstand;
    static const std::string escapedBlock;

    static void startGC();
    static void disassembleMethod();

    static VMFrame* popFrame();
    static void popFrameAndPushResult(vm_oop_t result);

    static void send(VMSymbol* signature, VMClass* receiverClass);

    static void triggerDoesNotUnderstand(VMSymbol* signature);

    static void doDup();
    static void doPushLocal(size_t bytecodeIndex);
    static void doPushLocalWithIndex(uint8_t localIndex);
    static void doReturnFieldWithIndex(uint8_t fieldIndex);
    static void doPushArgument(size_t bytecodeIndex);
    static void doPushField(size_t bytecodeIndex);
    static void doPushFieldWithIndex(uint8_t fieldIndex);
    static void doPushBlock(size_t bytecodeIndex);

    static inline void doPushConstant(size_t bytecodeIndex) {
        vm_oop_t constant = method->GetConstant(bytecodeIndex);
        GetFrame()->Push(constant);
    }

    static void doPushGlobal(size_t bytecodeIndex);
    static void doPop();
    static void doPopLocal(size_t bytecodeIndex);
    static void doPopLocalWithIndex(uint8_t localIndex);
    static void doPopArgument(size_t bytecodeIndex);
    static void doPopField(size_t bytecodeIndex);
    static void doPopFieldWithIndex(uint8_t fieldIndex);
    static void doSend(size_t bytecodeIndex);
    static void doUnarySend(size_t bytecodeIndex);
    static void doSuperSend(size_t bytecodeIndex);
    static void doReturnLocal();
    static void doReturnNonLocal();
    static void doInc();
    static void doDec();
    static void doIncField(uint8_t fieldIndex);
    static void doIncFieldPush(uint8_t fieldIndex);
    static bool checkIsGreater();
};
