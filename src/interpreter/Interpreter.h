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

#ifdef USE_YK
#include "../vm/Yk.h"
#endif

// Yk requires exactly one call site for yk_mt_control_point in the binary.
// DISPATCH_NOGC/GC therefore jump to a trampoline label (YK_DISPATCH_START)
// where the single control point call lives.  The trampoline is defined in
// Interpreter::Start() using the YK_DISPATCH_TRAMPOLINE macro below.
#ifdef USE_YK
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define DISPATCH_NOGC() goto YK_DISPATCH_START
#define DISPATCH_GC()                                       \
    {                                                       \
        if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) { \
            startGC();                                      \
        }                                                   \
        goto YK_DISPATCH_START;                             \
    }
// Switch-based dispatch for USE_YK: computed gotos (goto*) compile to LLVM
// indirectbr which Yk's tracer cannot trace through. A switch compiles to a
// regular br with multiple successors and is fully traceable.
#define YK_DISPATCH_TRAMPOLINE()                                             \
    YK_DISPATCH_START:                                                       \
        yk_mt_control_point(global_yk_mt,                                    \
            &static_cast<YkLocation*>(method->yklocs)[bytecodeIndexGlobal]); \
        switch (currentBytecodes[bytecodeIndexGlobal]) {                     \
            case BC_HALT:                  goto LABEL_BC_HALT;               \
            case BC_DUP:                   goto LABEL_BC_DUP;                \
            case BC_DUP_SECOND:            goto LABEL_BC_DUP_SECOND;         \
            case BC_PUSH_LOCAL:            goto LABEL_BC_PUSH_LOCAL;         \
            case BC_PUSH_LOCAL_0:          goto LABEL_BC_PUSH_LOCAL_0;       \
            case BC_PUSH_LOCAL_1:          goto LABEL_BC_PUSH_LOCAL_1;       \
            case BC_PUSH_LOCAL_2:          goto LABEL_BC_PUSH_LOCAL_2;       \
            case BC_PUSH_ARGUMENT:         goto LABEL_BC_PUSH_ARGUMENT;      \
            case BC_PUSH_SELF:             goto LABEL_BC_PUSH_SELF;          \
            case BC_PUSH_ARG_1:            goto LABEL_BC_PUSH_ARG_1;         \
            case BC_PUSH_ARG_2:            goto LABEL_BC_PUSH_ARG_2;         \
            case BC_PUSH_FIELD:            goto LABEL_BC_PUSH_FIELD;         \
            case BC_PUSH_FIELD_0:          goto LABEL_BC_PUSH_FIELD_0;       \
            case BC_PUSH_FIELD_1:          goto LABEL_BC_PUSH_FIELD_1;       \
            case BC_PUSH_BLOCK:            goto LABEL_BC_PUSH_BLOCK;         \
            case BC_PUSH_CONSTANT:         goto LABEL_BC_PUSH_CONSTANT;      \
            case BC_PUSH_CONSTANT_0:       goto LABEL_BC_PUSH_CONSTANT_0;    \
            case BC_PUSH_CONSTANT_1:       goto LABEL_BC_PUSH_CONSTANT_1;    \
            case BC_PUSH_CONSTANT_2:       goto LABEL_BC_PUSH_CONSTANT_2;    \
            case BC_PUSH_0:                goto LABEL_BC_PUSH_0;             \
            case BC_PUSH_1:                goto LABEL_BC_PUSH_1;             \
            case BC_PUSH_NIL:              goto LABEL_BC_PUSH_NIL;           \
            case BC_PUSH_GLOBAL:           goto LABEL_BC_PUSH_GLOBAL;        \
            case BC_POP:                   goto LABEL_BC_POP;                \
            case BC_POP_LOCAL:             goto LABEL_BC_POP_LOCAL;          \
            case BC_POP_LOCAL_0:           goto LABEL_BC_POP_LOCAL_0;        \
            case BC_POP_LOCAL_1:           goto LABEL_BC_POP_LOCAL_1;        \
            case BC_POP_LOCAL_2:           goto LABEL_BC_POP_LOCAL_2;        \
            case BC_POP_ARGUMENT:          goto LABEL_BC_POP_ARGUMENT;       \
            case BC_POP_FIELD:             goto LABEL_BC_POP_FIELD;          \
            case BC_POP_FIELD_0:           goto LABEL_BC_POP_FIELD_0;        \
            case BC_POP_FIELD_1:           goto LABEL_BC_POP_FIELD_1;        \
            case BC_SEND:                  goto LABEL_BC_SEND;               \
            case BC_SEND_1:                goto LABEL_BC_SEND_1;             \
            case BC_SUPER_SEND:            goto LABEL_BC_SUPER_SEND;         \
            case BC_RETURN_LOCAL:          goto LABEL_BC_RETURN_LOCAL;       \
            case BC_RETURN_NON_LOCAL:      goto LABEL_BC_RETURN_NON_LOCAL;   \
            case BC_RETURN_SELF:           goto LABEL_BC_RETURN_SELF;        \
            case BC_RETURN_FIELD_0:        goto LABEL_BC_RETURN_FIELD_0;     \
            case BC_RETURN_FIELD_1:        goto LABEL_BC_RETURN_FIELD_1;     \
            case BC_RETURN_FIELD_2:        goto LABEL_BC_RETURN_FIELD_2;     \
            case BC_INC:                   goto LABEL_BC_INC;                \
            case BC_DEC:                   goto LABEL_BC_DEC;                \
            case BC_INC_FIELD:             goto LABEL_BC_INC_FIELD;          \
            case BC_INC_FIELD_PUSH:        goto LABEL_BC_INC_FIELD_PUSH;     \
            case BC_JUMP:                  goto LABEL_BC_JUMP;               \
            case BC_JUMP_ON_FALSE_POP:     goto LABEL_BC_JUMP_ON_FALSE_POP;  \
            case BC_JUMP_ON_TRUE_POP:      goto LABEL_BC_JUMP_ON_TRUE_POP;   \
            case BC_JUMP_ON_FALSE_TOP_NIL: goto LABEL_BC_JUMP_ON_FALSE_TOP_NIL; \
            case BC_JUMP_ON_TRUE_TOP_NIL:  goto LABEL_BC_JUMP_ON_TRUE_TOP_NIL;  \
            case BC_JUMP_ON_NOT_NIL_POP:   goto LABEL_BC_JUMP_ON_NOT_NIL_POP;   \
            case BC_JUMP_ON_NIL_POP:       goto LABEL_BC_JUMP_ON_NIL_POP;       \
            case BC_JUMP_ON_NOT_NIL_TOP_TOP: goto LABEL_BC_JUMP_ON_NOT_NIL_TOP_TOP; \
            case BC_JUMP_ON_NIL_TOP_TOP:   goto LABEL_BC_JUMP_ON_NIL_TOP_TOP;   \
            case BC_JUMP_IF_GREATER:       goto LABEL_BC_JUMP_IF_GREATER;    \
            case BC_JUMP_BACKWARD:         goto LABEL_BC_JUMP_BACKWARD;      \
            case BC_JUMP2:                 goto LABEL_BC_JUMP2;              \
            case BC_JUMP2_ON_FALSE_POP:    goto LABEL_BC_JUMP2_ON_FALSE_POP; \
            case BC_JUMP2_ON_TRUE_POP:     goto LABEL_BC_JUMP2_ON_TRUE_POP;  \
            case BC_JUMP2_ON_FALSE_TOP_NIL: goto LABEL_BC_JUMP2_ON_FALSE_TOP_NIL; \
            case BC_JUMP2_ON_TRUE_TOP_NIL:  goto LABEL_BC_JUMP2_ON_TRUE_TOP_NIL;  \
            case BC_JUMP2_ON_NOT_NIL_POP:   goto LABEL_BC_JUMP2_ON_NOT_NIL_POP;   \
            case BC_JUMP2_ON_NIL_POP:       goto LABEL_BC_JUMP2_ON_NIL_POP;       \
            case BC_JUMP2_ON_NOT_NIL_TOP_TOP: goto LABEL_BC_JUMP2_ON_NOT_NIL_TOP_TOP; \
            case BC_JUMP2_ON_NIL_TOP_TOP:   goto LABEL_BC_JUMP2_ON_NIL_TOP_TOP;   \
            case BC_JUMP2_IF_GREATER:      goto LABEL_BC_JUMP2_IF_GREATER;   \
            case BC_JUMP2_BACKWARD:        goto LABEL_BC_JUMP2_BACKWARD;     \
            default: __builtin_unreachable();                                 \
        }
// NOLINTEND(cppcoreguidelines-macro-usage)
#else
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
#define YK_DISPATCH_TRAMPOLINE() (void)0
#endif

class Interpreter {
public:
    static vm_oop_t Start(bool printBytecodes = false);

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

    static constexpr const char* unknownGlobal = "unknownGlobal:";
    static constexpr const char* doesNotUnderstand = "doesNotUnderstand:arguments:";
    static constexpr const char* escapedBlock = "escapedBlock:";

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
