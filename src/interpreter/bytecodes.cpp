#include "bytecodes.h"

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

#include <cassert>
#include <cstdint>

const uint8_t Bytecode::bytecodeLengths[] = {
    1,  // BC_HALT
    1,  // BC_DUP
    1,  // BC_DUP_SECOND
    3,  // BC_PUSH_LOCAL
    1,  // BC_PUSH_LOCAL_0
    1,  // BC_PUSH_LOCAL_1
    1,  // BC_PUSH_LOCAL_2
    3,  // BC_PUSH_ARGUMENT
    1,  // BC_PUSH_SELF
    1,  // BC_PUSH_ARG_1
    1,  // BC_PUSH_ARG_2
    2,  // BC_PUSH_FIELD
    1,  // BC_PUSH_FIELD_0
    1,  // BC_PUSH_FIELD_1
    2,  // BC_PUSH_BLOCK
    2,  // BC_PUSH_CONSTANT
    1,  // BC_PUSH_CONSTANT_0
    1,  // BC_PUSH_CONSTANT_1
    1,  // BC_PUSH_CONSTANT_2
    1,  // BC_PUSH_0
    1,  // BC_PUSH_1
    1,  // BC_PUSH_NIL
    2,  // BC_PUSH_GLOBAL
    1,  // BC_POP
    3,  // BC_POP_LOCAL
    1,  // BC_POP_LOCAL_0
    1,  // BC_POP_LOCAL_1
    1,  // BC_POP_LOCAL_2
    3,  // BC_POP_ARGUMENT
    2,  // BC_POP_FIELD
    1,  // BC_POP_FIELD_0
    1,  // BC_POP_FIELD_1
    2,  // BC_SEND
    2,  // BC_SEND_1
    2,  // BC_SUPER_SEND
    1,  // BC_RETURN_LOCAL
    1,  // BC_RETURN_NON_LOCAL
    1,  // BC_RETURN_SELF
    1,  // BC_RETURN_FIELD_0
    1,  // BC_RETURN_FIELD_1
    1,  // BC_RETURN_FIELD_2
    1,  // BC_INC
    1,  // BC_DEC
    2,  // BC_INC_FIELD
    2,  // BC_INC_FIELD_PUSH

    3,  // BC_JUMP
    3,  // BC_JUMP_ON_FALSE_POP
    3,  // BC_JUMP_ON_TRUE_POP
    3,  // BC_JUMP_ON_FALSE_TOP_NIL
    3,  // BC_JUMP_ON_TRUE_TOP_NIL
    3,  // BC_JUMP_ON_NOT_NIL_POP
    3,  // BC_JUMP_ON_NIL_POP
    3,  // BC_JUMP_ON_NOT_NIL_TOP_TOP
    3,  // BC_JUMP_ON_NIL_TOP_TOP
    3,  // BC_JUMP_IF_GREATER
    3,  // BC_JUMP_BACKWARD

    3,  // BC_JUMP2
    3,  // BC_JUMP2_ON_FALSE_POP
    3,  // BC_JUMP2_ON_TRUE_POP
    3,  // BC_JUMP2_ON_FALSE_TOP_NIL
    3,  // BC_JUMP2_ON_TRUE_TOP_NIL
    3,  // BC_JUMP2_ON_NOT_NIL_POP
    3,  // BC_JUMP2_ON_NIL_POP
    3,  // BC_JUMP2_ON_NOT_NIL_TOP_TOP
    3,  // BC_JUMP2_ON_NIL_TOP_TOP
    3,  // BC_JUMP2_IF_GREATER
    3,  // BC_JUMP2_BACKWARD

    2,  // BC_PUSH_BLOCK_WITHOUT_CONTEXT
};

const char* Bytecode::bytecodeNames[] = {
    "HALT            ",          // 0
    "DUP             ",          // 1
    "DUP_SECOND      ",          // 2
    "PUSH_LOCAL      ",          // 3
    "PUSH_LOCAL_0    ",          // 4
    "PUSH_LOCAL_1    ",          // 5
    "PUSH_LOCAL_2    ",          // 6
    "PUSH_ARGUMENT   ",          // 7
    "PUSH_SELF       ",          // 8
    "PUSH_ARG_1      ",          // 9
    "PUSH_ARG_2      ",          // 10
    "PUSH_FIELD      ",          // 11
    "PUSH_FIELD_0    ",          // 12
    "PUSH_FIELD_1    ",          // 13
    "PUSH_BLOCK      ",          // 14
    "PUSH_CONSTANT   ",          // 15
    "PUSH_CONSTANT_0 ",          // 16
    "PUSH_CONSTANT_1 ",          // 17
    "PUSH_CONSTANT_2 ",          // 18
    "PUSH_0          ",          // 19
    "PUSH_1          ",          // 20
    "PUSH_NIL        ",          // 21
    "PUSH_GLOBAL     ",          // 22
    "POP             ",          // 23
    "POP_LOCAL       ",          // 24
    "POP_LOCAL_0     ",          // 25
    "POP_LOCAL_1     ",          // 26
    "POP_LOCAL_2     ",          // 27
    "POP_ARGUMENT    ",          // 28
    "POP_FIELD       ",          // 29
    "POP_FIELD_0     ",          // 30
    "POP_FIELD_1     ",          // 31
    "SEND            ",          // 32
    "SEND_1          ",          // 33
    "SUPER_SEND      ",          // 34
    "RETURN_LOCAL    ",          // 35
    "RETURN_NON_LOCAL",          // 36
    "RETURN_SELF     ",          // 37
    "RETURN_FIELD_0  ",          // 38
    "RETURN_FIELD_1  ",          // 39
    "RETURN_FIELD_2  ",          // 40
    "INC             ",          // 41
    "DEC             ",          // 42
    "INC_FIELD       ",          // 43
    "INC_FIELD_PUSH  ",          // 44
    "JUMP            ",          // 45
    "JUMP_ON_FALSE_POP",         // 46
    "JUMP_ON_TRUE_POP",          // 47
    "JUMP_ON_FALSE_TOP_NIL",     // 48
    "JUMP_ON_TRUE_TOP_NIL",      // 49
    "JUMP_ON_NOT_NIL_POP",       // 50
    "JUMP_ON_NIL_POP ",          // 51
    "JUMP_ON_NOT_NIL_TOP_TOP",   // 52
    "JUMP_ON_NIL_TOP_TOP",       // 53
    "JUMP_IF_GREATER ",          // 54
    "JUMP_BACKWARD   ",          // 55
    "JUMP2           ",          // 56
    "JUMP2_ON_FALSE_POP",        // 57
    "JUMP2_ON_TRUE_POP",         // 58
    "JUMP2_ON_FALSE_TOP_NIL",    // 59
    "JUMP2_ON_TRUE_TOP_NIL",     // 60
    "JUMP2_ON_NOT_NIL_POP",      // 61
    "JUMP2_ON_NIL_POP ",         // 62
    "JUMP2_ON_NOT_NIL_TOP_TOP",  // 63
    "JUMP2_ON_NIL_TOP_TOP",      // 64
    "JUMP2_IF_GREATER",          // 65
    "JUMP2_BACKWARD  ",          // 66
    "PUSH_BLOCK_NO_CTX",         // 67
};

bool IsJumpBytecode(uint8_t bc) {
    static_assert(
        BC_JUMP < BC_JUMP2_BACKWARD,
        "make sure the nummeric value of jump bytecodes is as expected");
    static_assert((BC_JUMP2_BACKWARD - BC_JUMP) == 21,
                  "we expect there to be 22 jump bytecodes");

    return BC_JUMP <= bc && bc <= BC_JUMP2_BACKWARD;
}

uint8_t IsPushConstBytecode(uint8_t bc) {
    switch (bc) {
        case BC_PUSH_CONSTANT:
        case BC_PUSH_CONSTANT_0:
        case BC_PUSH_CONSTANT_1:
        case BC_PUSH_CONSTANT_2:
        case BC_PUSH_0:
        case BC_PUSH_1:
        case BC_PUSH_NIL:
            return bc;

        default:
            return BC_INVALID;
    }
}

uint8_t IsPushArgBytecode(uint8_t bc) {
    switch (bc) {
        case BC_PUSH_SELF:
        case BC_PUSH_ARG_1:
        case BC_PUSH_ARG_2:
        case BC_PUSH_ARGUMENT:
            return bc;

        default:
            return BC_INVALID;
    }
}

uint8_t IsPushFieldBytecode(uint8_t bc) {
    switch (bc) {
        case BC_PUSH_FIELD:
        case BC_PUSH_FIELD_0:
        case BC_PUSH_FIELD_1:
            return bc;

        default:
            return BC_INVALID;
    }
}

uint8_t IsPopFieldBytecode(uint8_t bc) {
    switch (bc) {
        case BC_POP_FIELD:
        case BC_POP_FIELD_0:
        case BC_POP_FIELD_1:
            return bc;

        default:
            return BC_INVALID;
    }
}

uint8_t IsReturnFieldBytecode(uint8_t bc) {
    switch (bc) {
        case BC_RETURN_FIELD_0:
        case BC_RETURN_FIELD_1:
        case BC_RETURN_FIELD_2:
            return bc;

        default:
            return BC_INVALID;
    }
}

uint8_t IsPopSmthBytecode(uint8_t bc) {
    switch (bc) {
        case BC_POP_LOCAL:
        case BC_POP_LOCAL_0:
        case BC_POP_LOCAL_1:
        case BC_POP_LOCAL_2:
        case BC_POP_ARGUMENT:
        case BC_POP_FIELD:
        case BC_POP_FIELD_0:
        case BC_POP_FIELD_1:
            return bc;

        default:
            return BC_INVALID;
    }
}

bool Bytecode::BytecodeDefinitionsAreConsistent() {
    bool const namesAndLengthMatch =
        (sizeof(Bytecode::bytecodeNames) / sizeof(char*)) ==
        (sizeof(Bytecode::bytecodeLengths) / sizeof(uint8_t));
    bool const lastBytecodeLinesUp =
        _LAST_BYTECODE ==
        (sizeof(Bytecode::bytecodeLengths) - 1);  // -1 because null terminated

    return namesAndLengthMatch && lastBytecodeLinesUp;
}
