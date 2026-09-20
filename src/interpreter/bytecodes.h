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

#include <cassert>

#include "../misc/defs.h"

// bytecode constants used by SOM++

// clang-format off
#define BC_HALT                    0
#define BC_DUP                     1
#define BC_DUP_SECOND              2
#define BC_PUSH_LOCAL              3
#define BC_PUSH_LOCAL_0            4
#define BC_PUSH_LOCAL_1            5
#define BC_PUSH_LOCAL_2            6
#define BC_PUSH_ARGUMENT           7
#define BC_PUSH_SELF               8
#define BC_PUSH_ARG_1              9
#define BC_PUSH_ARG_2             10
#define BC_PUSH_FIELD             11
#define BC_PUSH_FIELD_0           12
#define BC_PUSH_FIELD_1           13
#define BC_PUSH_BLOCK             14
#define BC_PUSH_CONSTANT          15
#define BC_PUSH_CONSTANT_0        16
#define BC_PUSH_CONSTANT_1        17
#define BC_PUSH_CONSTANT_2        18
#define BC_PUSH_0                 19
#define BC_PUSH_1                 20
#define BC_PUSH_NIL               21
#define BC_PUSH_GLOBAL            22
#define BC_POP                    23
#define BC_POP_LOCAL              24
#define BC_POP_LOCAL_0            25
#define BC_POP_LOCAL_1            26
#define BC_POP_LOCAL_2            27
#define BC_POP_ARGUMENT           28
#define BC_POP_FIELD              29
#define BC_POP_FIELD_0            30
#define BC_POP_FIELD_1            31
#define BC_SEND                   32
#define BC_SEND_1                 33
#define BC_SUPER_SEND             34
#define BC_RETURN_LOCAL           35
#define BC_RETURN_NON_LOCAL       36
#define BC_RETURN_SELF            37
#define BC_RETURN_FIELD_0         38
#define BC_RETURN_FIELD_1         39
#define BC_RETURN_FIELD_2         40
#define BC_INC                    41
#define BC_DEC                    42
#define BC_INC_FIELD              43
#define BC_INC_FIELD_PUSH         44
#define BC_JUMP                   45
#define BC_JUMP_ON_FALSE_POP      46
#define BC_JUMP_ON_TRUE_POP       47
#define BC_JUMP_ON_FALSE_TOP_NIL  48
#define BC_JUMP_ON_TRUE_TOP_NIL   49
#define BC_JUMP_ON_NOT_NIL_POP    50
#define BC_JUMP_ON_NIL_POP        51
#define BC_JUMP_ON_NOT_NIL_TOP_TOP 52
#define BC_JUMP_ON_NIL_TOP_TOP    53
#define BC_JUMP_IF_GREATER        54
#define BC_JUMP_BACKWARD          55
#define BC_JUMP2                  56
#define BC_JUMP2_ON_FALSE_POP     57
#define BC_JUMP2_ON_TRUE_POP      58
#define BC_JUMP2_ON_FALSE_TOP_NIL 59
#define BC_JUMP2_ON_TRUE_TOP_NIL  60
#define BC_JUMP2_ON_NOT_NIL_POP   61
#define BC_JUMP2_ON_NIL_POP       62
#define BC_JUMP2_ON_NOT_NIL_TOP_TOP 63
#define BC_JUMP2_ON_NIL_TOP_TOP   64
#define BC_JUMP2_IF_GREATER       65
#define BC_JUMP2_BACKWARD         66
#define BC_PUSH_BLOCK_WITHOUT_CONTEXT 67

#define _LAST_BYTECODE BC_PUSH_BLOCK_WITHOUT_CONTEXT

#define BC_INVALID           255
// clang-format on

// TODO(smarr): port support for these bytecodes
//       they were already named in ported code, and it seemed nicer to just
//       already include that code
// clang-format off
#define BC_SEND_N            250
#define BC_SEND_3            249
#define BC_SEND_2            248
// clang-format on

// properties of the bytecodes

#define FIRST_DOUBLE_BYTE_JUMP_BYTECODE BC_JUMP2
#define NUM_SINGLE_BYTE_JUMP_BYTECODES ((BC_JUMP_BACKWARD - BC_JUMP) + 1)

class Bytecode {
public:
    static char* GetBytecodeName(uint8_t bc) {
        return (char*)bytecodeNames[bc];
    }

    inline static uint8_t GetBytecodeLength(uint8_t bc) {
        return bytecodeLengths[bc];  // Return the length of the given bytecode
    }

    static bool BytecodeDefinitionsAreConsistent();

private:
    static const uint8_t bytecodeLengths[];

    static const char* bytecodeNames[];
};

inline uint16_t ComputeOffset(uint8_t byte1, uint8_t byte2) {
    return ((uint16_t)byte1) | (uint16_t)(((uint16_t)byte2) << 8U);
}

bool IsJumpBytecode(uint8_t bc);
uint8_t IsPushConstBytecode(uint8_t bc);
uint8_t IsPushFieldBytecode(uint8_t bc);
uint8_t IsPushArgBytecode(uint8_t bc);
uint8_t IsPopFieldBytecode(uint8_t bc);
uint8_t IsPopSmthBytecode(uint8_t bc);
uint8_t IsReturnFieldBytecode(uint8_t bc);
