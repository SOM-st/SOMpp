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

#include "BytecodeGenerator.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "../interpreter/bytecodes.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "../vm/Symbols.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/Signature.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMSymbol.h"
#include "MethodGenerationContext.h"
#include "Parser.h"

void Emit1(MethodGenerationContext& mgenc, uint8_t bytecode,
           int64_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
}

void Emit2(MethodGenerationContext& mgenc, uint8_t bytecode, uint8_t idx,
           int64_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    mgenc.AddBytecodeArgument(idx);
}

void Emit3(MethodGenerationContext& mgenc, uint8_t bytecode, uint8_t idx,
           size_t ctx, int64_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    mgenc.AddBytecodeArgument(idx);
    mgenc.AddBytecodeArgument(ctx);
}

void EmitHALT(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_HALT, 0);
}

void EmitDUP(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_DUP, 1);
}

void EmitPUSHLOCAL(MethodGenerationContext& mgenc, const Parser& parser,
                   size_t index, size_t context) {
    if (index > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method has too many local variables. You may be able to split "
            "up this method into multiple to avoid the issue.");
    }

    if (context > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "This block is too deeply nested. You may be able to split up this "
            "method into multiple to avoid the issue.");
    }

    const uint8_t idx = index;
    const uint8_t ctx = context;

    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_PUSH_LOCAL_0, 1);
            return;
        }
        if (idx == 1) {
            Emit1(mgenc, BC_PUSH_LOCAL_1, 1);
            return;
        }
        if (idx == 2) {
            Emit1(mgenc, BC_PUSH_LOCAL_2, 1);
            return;
        }
    }
    Emit3(mgenc, BC_PUSH_LOCAL, idx, ctx, 1);
}

void EmitPUSHARGUMENT(MethodGenerationContext& mgenc, const Parser& parser,
                      size_t index, size_t context) {
    if (index > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method has too many arguments. You may be able to split up "
            "this method into multiple to avoid the issue.");
    }

    if (context > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "This block is too deeply nested. You may be able to split up this "
            "method into multiple to avoid the issue.");
    }

    const uint8_t idx = index;
    const uint8_t ctx = context;

    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_PUSH_SELF, 1);
            return;
        }

        if (idx == 1) {
            Emit1(mgenc, BC_PUSH_ARG_1, 1);
            return;
        }

        if (idx == 2) {
            Emit1(mgenc, BC_PUSH_ARG_2, 1);
            return;
        }
    }
    Emit3(mgenc, BC_PUSH_ARGUMENT, idx, ctx, 1);
}

void EmitPUSHFIELD(MethodGenerationContext& mgenc, const Parser& parser,
                   VMSymbol* field) {
    const int64_t idx = mgenc.GetFieldIndex(field);
    if (idx < 0) {
        std::string const msg(
            "The method tries to access the field " + field->GetStdString() +
            ", which was not found in the class " +
            mgenc.GetHolder()->GetName()->GetStdString() + ".");
        parser.ParseError(msg.c_str());
    }

    if (idx > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method tries to access a field that cannot be represented in "
            "the SOM++ bytecodes. Make sure this class and its superclasses "
            "have less than 256 fields in total.");
    }

    EmitPushFieldWithIndex(mgenc, idx);
}

void EmitPUSHBLOCK(MethodGenerationContext& mgenc, const Parser& parser,
                   VMInvokable* block) {
    const uint8_t idx = mgenc.AddLiteralIfAbsent(block, parser);
    Emit2(mgenc, BC_PUSH_BLOCK, idx, 1);
}

void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, const Parser& parser,
                      vm_oop_t cst) {
    // this isn't very robust with respect to initialization order
    // so, we check here, and hope it's working, but alternatively
    // we also make sure that we don't miss anything in the else
    // branch of the class check
    if (CLASS_OF(cst) == load_ptr(integerClass)) {
        if (SMALL_INT_VAL(cst) == 0LL) {
            Emit1(mgenc, BC_PUSH_0, 1);
            return;
        }
        if (SMALL_INT_VAL(cst) == 1LL) {
            Emit1(mgenc, BC_PUSH_1, 1);
            return;
        }
    } else {
        assert(!IsVMInteger(cst));
    }

    if (cst == load_ptr(nilObject)) {
        Emit1(mgenc, BC_PUSH_NIL, 1);
        return;
    }

    const uint8_t idx = mgenc.AddLiteralIfAbsent(cst, parser);
    if (idx == 0) {
        Emit1(mgenc, BC_PUSH_CONSTANT_0, 1);
        return;
    }
    if (idx == 1) {
        Emit1(mgenc, BC_PUSH_CONSTANT_1, 1);
        return;
    }
    if (idx == 2) {
        Emit1(mgenc, BC_PUSH_CONSTANT_2, 1);
        return;
    }

    Emit2(mgenc, BC_PUSH_CONSTANT, idx, 1);
}

void EmitPUSHCONSTANT(MethodGenerationContext& mgenc, uint8_t literalIndex) {
    Emit2(mgenc, BC_PUSH_CONSTANT, literalIndex, 1);
}

void EmitPUSHCONSTANTString(MethodGenerationContext& mgenc, VMString* str) {
    Emit2(mgenc, BC_PUSH_CONSTANT, mgenc.FindLiteralIndex(str), 1);
}

void EmitPUSHGLOBAL(MethodGenerationContext& mgenc, const Parser& parser,
                    VMSymbol* global) {
    if (global == SymbolFor("nil")) {
        EmitPUSHCONSTANT(mgenc, parser, load_ptr(nilObject));
    } else if (global == SymbolFor("true")) {
        EmitPUSHCONSTANT(mgenc, parser, load_ptr(trueObject));
    } else if (global == SymbolFor("false")) {
        EmitPUSHCONSTANT(mgenc, parser, load_ptr(falseObject));
    } else {
        const uint8_t idx = mgenc.AddLiteralIfAbsent(global, parser);
        Emit2(mgenc, BC_PUSH_GLOBAL, idx, 1);
    }
}

void EmitPOP(MethodGenerationContext& mgenc) {
    if (!mgenc.OptimizeDupPopPopSequence()) {
        Emit1(mgenc, BC_POP, -1);
    }
}

void EmitPOPLOCAL(MethodGenerationContext& mgenc, const Parser& parser,
                  size_t index, size_t context) {
    if (index > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method has too many local variables. You may be able to split "
            "up this method into multiple to avoid the issue.");
    }

    if (context > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "This block is too deeply nested. You may be able to split up this "
            "method into multiple to avoid the issue.");
    }

    const uint8_t idx = index;
    const uint8_t ctx = context;

    if (ctx == 0) {
        if (idx == 0) {
            Emit1(mgenc, BC_POP_LOCAL_0, -1);
            return;
        }

        if (idx == 1) {
            Emit1(mgenc, BC_POP_LOCAL_1, -1);
            return;
        }

        if (idx == 2) {
            Emit1(mgenc, BC_POP_LOCAL_2, -1);
            return;
        }
    }

    Emit3(mgenc, BC_POP_LOCAL, idx, ctx, -1);
}

void EmitPOPARGUMENT(MethodGenerationContext& mgenc, const Parser& parser,
                     size_t idx, size_t ctx) {
    if (idx > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method has too many arguments. You may be able to split up "
            "this method into multiple to avoid the issue.");
    }

    if (ctx > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "This block is too deeply nested. You may be able to split up this "
            "method into multiple to avoid the issue.");
    }

    Emit3(mgenc, BC_POP_ARGUMENT, idx, ctx, -1);
}

void EmitPOPFIELD(MethodGenerationContext& mgenc, const Parser& parser,
                  VMSymbol* field) {
    const int64_t idx = mgenc.GetFieldIndex(field);
    if (idx < 0) {
        std::string const msg(
            "The method tries to access the field " + field->GetStdString() +
            ", which was not found in the class " +
            mgenc.GetHolder()->GetName()->GetStdString() + ".");
        parser.ParseError(msg.c_str());
    }

    if (idx > std::numeric_limits<uint8_t>::max()) {
        parser.ParseError(
            "The method tries to access a field that cannot be represented in "
            "the SOM++ bytecodes. Make sure this class and its superclasses "
            "have less than 256 fields in total.");
    }

    if (mgenc.OptimizeIncField(idx)) {
        return;
    }

    EmitPopFieldWithIndex(mgenc, idx);
}

void EmitSEND(MethodGenerationContext& mgenc, const Parser& parser,
              VMSymbol* msg) {
    const uint8_t idx = mgenc.AddLiteralIfAbsent(msg, parser);

    const uint8_t numArgs = Signature::GetNumberOfArguments(msg);
    const int64_t stackEffect = -numArgs + 1;  // +1 for the result

    Emit2(mgenc, numArgs == 1 ? BC_SEND_1 : BC_SEND, idx, stackEffect);
}

void EmitSUPERSEND(MethodGenerationContext& mgenc, const Parser& parser,
                   VMSymbol* msg) {
    const uint8_t idx = mgenc.AddLiteralIfAbsent(msg, parser);
    const uint8_t numArgs = Signature::GetNumberOfArguments(msg);
    const int64_t stackEffect = -numArgs + 1;  // +1 for the result

    Emit2(mgenc, BC_SUPER_SEND, idx, stackEffect);
}

void EmitRETURNSELF(MethodGenerationContext& mgenc) {
    mgenc.OptimizeDupPopPopSequence();
    Emit1(mgenc, BC_RETURN_SELF, 0);
}

void EmitRETURNLOCAL(MethodGenerationContext& mgenc, const Parser& parser) {
    if (!mgenc.OptimizeReturnField(parser)) {
        Emit1(mgenc, BC_RETURN_LOCAL, 0);
    }
}

void EmitRETURNNONLOCAL(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_RETURN_NON_LOCAL, 0);
}

void EmitRETURNFIELD(MethodGenerationContext& mgenc, const Parser& parser,
                     size_t index) {
    if (index > 2) {
        parser.ParseError(
            "Internal Error: EmitRETURNFIELD has unsupported argument");
    }

    uint8_t bc = 0;
    switch (index) {
        case 0:
            bc = BC_RETURN_FIELD_0;
            break;
        case 1:
            bc = BC_RETURN_FIELD_1;
            break;
        case 2:
            bc = BC_RETURN_FIELD_2;
            break;
        default:
            bc = 0;
            break;
    }
    Emit1(mgenc, bc, 0);
}

void EmitINC(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_INC, 0);
}

void EmitDEC(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_DEC, 0);
}

void EmitIncFieldPush(MethodGenerationContext& mgenc, uint8_t fieldIdx) {
    Emit2(mgenc, BC_INC_FIELD_PUSH, fieldIdx, 1);
}

void EmitDupSecond(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_DUP_SECOND, 1);
}

size_t EmitJumpOnWithDummyOffset(MethodGenerationContext& mgenc,
                                 JumpCondition condition, bool needsPop) {
    // Remember: true and false seem flipped here.
    // This is because if the test passes, the block is inlined directly.
    // But if the test fails, we need to jump.
    // Thus, an  `#ifTrue:` needs to generated a jump_on_false.
    uint8_t bc = 0;
    int64_t const stackEffect = needsPop ? -1 : 0;

    switch (condition) {
        case JumpCondition::ON_TRUE:
            bc = needsPop ? BC_JUMP_ON_TRUE_POP : BC_JUMP_ON_TRUE_TOP_NIL;
            break;

        case JumpCondition::ON_FALSE:
            bc = needsPop ? BC_JUMP_ON_FALSE_POP : BC_JUMP_ON_FALSE_TOP_NIL;
            break;

        case JumpCondition::ON_NIL:
            bc = needsPop ? BC_JUMP_ON_NIL_POP : BC_JUMP_ON_NIL_TOP_TOP;
            break;

        case JumpCondition::ON_NOT_NIL:
            bc = needsPop ? BC_JUMP_ON_NOT_NIL_POP : BC_JUMP_ON_NOT_NIL_TOP_TOP;
            break;
    }

    Emit1(mgenc, bc, stackEffect);

    size_t const idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

size_t EmitJumpWithDumyOffset(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_JUMP, 0);
    size_t const idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

size_t EmitJumpIfGreaterWithDummyOffset(MethodGenerationContext& mgenc) {
    Emit1(mgenc, BC_JUMP_IF_GREATER, 0);
    size_t const idx = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return idx;
}

void EmitJumpBackwardWithOffset(MethodGenerationContext& mgenc,
                                size_t jumpOffset) {
    uint8_t const jumpBytecode =
        jumpOffset <= 0xFF ? BC_JUMP_BACKWARD : BC_JUMP2_BACKWARD;
    Emit3(mgenc, jumpBytecode, jumpOffset & 0xFFU, jumpOffset >> 8U, 0);
}

size_t Emit3WithDummy(MethodGenerationContext& mgenc, uint8_t bytecode,
                      int64_t stackEffect) {
    mgenc.AddBytecode(bytecode, stackEffect);
    size_t const index = mgenc.AddBytecodeArgumentAndGetIndex(0);
    mgenc.AddBytecodeArgument(0);
    return index;
}

void EmitPushFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx) {
    // if (ctxLevel == 0) {
    if (fieldIdx == 0) {
        Emit1(mgenc, BC_PUSH_FIELD_0, 1);
        return;
    }

    if (fieldIdx == 1) {
        Emit1(mgenc, BC_PUSH_FIELD_1, 1);
        return;
    }
    // }

    Emit2(mgenc, BC_PUSH_FIELD, fieldIdx, 1);
}

void EmitPopFieldWithIndex(MethodGenerationContext& mgenc, uint8_t fieldIdx) {
    // if (ctxLevel == 0) {
    if (fieldIdx == 0) {
        Emit1(mgenc, BC_POP_FIELD_0, -1);
        return;
    }

    if (fieldIdx == 1) {
        Emit1(mgenc, BC_POP_FIELD_1, -1);
        return;
    }
    // }

    Emit2(mgenc, BC_POP_FIELD, fieldIdx, -1);
}
