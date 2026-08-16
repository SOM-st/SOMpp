#include "BytecodeGenerationTest.h"

#include <cassert>
#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <string>
#include <vector>

#include "../interpreter/bytecodes.h"
#include "../misc/StringUtil.h"
#include "../vmobjects/VMMethod.h"
#include "TestWithParsing.h"

void BytecodeGenerationTest::testEmptyMethodReturnsSelf() {
    auto bytecodes = methodToBytecode("test = ( )");

    check(bytecodes, {BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushConstant() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        0. 1. nil. #a. true. false.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_0, BC_POP, BC_PUSH_1, BC_POP, BC_PUSH_NIL, BC_POP,
           BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfPushConstantSame() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        #a. #b. #c. #d.
                                        true ifFalse: [ #a. #b. #c. #d. ]
                                      ) )""");
    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
                      BC_PUSH_CONSTANT_2, BC_POP, BC(BC_PUSH_CONSTANT, 3),
                      BC_POP, BC(BC_PUSH_CONSTANT, 4),
                      BC(BC_JUMP_ON_TRUE_TOP_NIL, 11, 0), BC_PUSH_CONSTANT_0,
                      BC_POP, BC_PUSH_CONSTANT_1, BC_POP, BC_PUSH_CONSTANT_2,
                      BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfPushConstantDifferent() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        #a. #b. #c. #d.
                                        true ifFalse: [ #e. #f. #g. #h. ]
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_POP,
           BC(BC_PUSH_CONSTANT, 4), BC(BC_JUMP_ON_TRUE_TOP_NIL, 14, 0),
           BC(BC_PUSH_CONSTANT, 5), BC_POP, BC(BC_PUSH_CONSTANT, 6), BC_POP,
           BC(BC_PUSH_CONSTANT, 7), BC_POP, BC(BC_PUSH_CONSTANT, 8),
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testExplicitReturnSelf() {
    auto bytecodes = methodToBytecode("test = ( ^ self )");

    check(bytecodes, {BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopArgumentPop() {
    auto bytecodes = methodToBytecode("test: arg = ( arg := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopArgumentPopImplicitReturnSelf() {
    auto bytecodes = methodToBytecode("test: arg = ( arg := 1 )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopLocalPop() {
    auto bytecodes =
        methodToBytecode("test = ( | local | local := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC_POP_LOCAL_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopField0Pop() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( field := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC_POP_FIELD_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldPop() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("field");
    auto bytecodes = methodToBytecode("test = ( field := 1. ^ self )");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_FIELD, 4), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldReturnSelf() {
    addField("field");
    auto bytecodes = methodToBytecode("test: val = ( field := val )");

    check(bytecodes, {BC_PUSH_ARG_1, BC_POP_FIELD_0, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testDupPopFieldNReturnSelf() {
    addField("a");
    addField("b");
    addField("c");
    addField("d");
    addField("e");
    addField("field");
    auto bytecodes = methodToBytecode("test: val = ( field := val )");

    check(bytecodes, {BC_PUSH_ARG_1, BC(BC_POP_FIELD, 5), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testSendDupPopFieldReturnLocal() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( ^ field := self method )");

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND_1, 0), BC_DUP, BC_POP_FIELD_0,
           BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testSendDupPopFieldReturnLocalPeriod() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( ^ field := self method. )");

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND_1, 0), BC_DUP, BC_POP_FIELD_0,
           BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentPopReturnArg() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1. arg ]");

    check(bytecodes, {BC_PUSH_1, BC(BC_POP_ARGUMENT, 1, 0), BC_PUSH_ARG_1,
                      BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentImplicitReturn() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1 ]");
    check(bytecodes,
          {BC_PUSH_1, BC_DUP, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopArgumentImplicitReturnDot() {
    auto bytecodes = blockToBytecode("[:arg | arg := 1. ]");
    check(bytecodes,
          {BC_PUSH_1, BC_DUP, BC(BC_POP_ARGUMENT, 1, 0), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopLocalReturnLocal() {
    auto bytecodes = blockToBytecode("[| local | local := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_LOCAL_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopFieldReturnLocal() {
    addField("field");
    auto bytecodes = blockToBytecode("[ field := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_FIELD_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockDupPopFieldReturnLocalDot() {
    addField("field");
    auto bytecodes = blockToBytecode("[ field := 1 ]");
    check(bytecodes, {BC_PUSH_1, BC_DUP, BC_POP_FIELD_0, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testPushLocalOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        | l1 l2 l3 l4 |
                                        l1. l2. l3. l4.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_LOCAL_0, BC_POP, BC_PUSH_LOCAL_1, BC_POP, BC_PUSH_LOCAL_2,
           BC_POP, BC(BC_PUSH_LOCAL, 3, 0), BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushArgOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test: a1 and: a2 and: a3 and: a4 = (
                                        self. a1. a2. a3. a4.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_SELF, BC_POP, BC_PUSH_ARG_1, BC_POP, BC_PUSH_ARG_2, BC_POP,
           BC(BC_PUSH_ARGUMENT, 3, 0), BC_POP, BC(BC_PUSH_ARGUMENT, 4, 0),
           BC_POP, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPushFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = methodToBytecode("test = ( f1. f2. f3. f4 )");
    check(bytecodes,
          {BC_PUSH_FIELD_0, BC_POP, BC_PUSH_FIELD_1, BC_POP,
           BC(BC_PUSH_FIELD, 2), BC_POP, BC(BC_PUSH_FIELD, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testBlockPushFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = blockToBytecode("[ f1. f2. f3. f4 ]");

    check(
        bytecodes,
        {BC_PUSH_FIELD_0, BC_POP, BC_PUSH_FIELD_1, BC_POP, BC(BC_PUSH_FIELD, 2),
         BC_POP, BC(BC_PUSH_FIELD, 3), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testPopLocalOpt() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        | l1 l2 l3 l4 l5 |
                                        l1 := 1.
                                        l2 := 1.
                                        l3 := 1.
                                        l4 := 1.
                                        l5 := 1.
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_1, BC_POP_LOCAL_0, BC_PUSH_1, BC_POP_LOCAL_1, BC_PUSH_1,
           BC_POP_LOCAL_2, BC_PUSH_1, BC(BC_POP_LOCAL, 3, 0), BC_PUSH_1,
           BC(BC_POP_LOCAL, 4, 0), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testPopFieldOpt() {
    addField("f1");
    addField("f2");
    addField("f3");
    addField("f4");
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        f1 := 1.
                                        f2 := 1.
                                        f3 := 1.
                                        f4 := 1.
                                      ) )""");
    check(
        bytecodes,
        {BC_PUSH_1, BC_POP_FIELD_0, BC_PUSH_1, BC_POP_FIELD_1, BC_PUSH_1,
         BC(BC_POP_FIELD, 2), BC_PUSH_1, BC(BC_POP_FIELD, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testWhileInlining(const char* selector,
                                               uint8_t jumpBytecode) {
    std::string source = R"""(   test: arg = (
                                      #start.
                                      [ true ] SELECTOR [ arg ].
                                      #end
                                  ) )""";
    bool const wasReplaced = ReplacePattern(source, "SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(
        bytecodes,
        {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC(jumpBytecode, 8, 0),
         BC_PUSH_ARG_1, BC_POP, BC(BC_JUMP_BACKWARD, 6, 0), BC_PUSH_NIL, BC_POP,
         BC_PUSH_CONSTANT_2, BC_RETURN_SELF});
}

/** This test checks whether the jumps in the while loop are correct after it
 * got inlined. */
void BytecodeGenerationTest::testInliningWhileLoopsWithExpandingBranches() {
    auto bytecodes = methodToBytecode(R"""(
        test = (
          #const0. #const1. #const2.
          0 ifTrue: [
            [ #const3. #const4. #const5 ]
               whileTrue: [
                 #const6. #const7. #const8 ]
          ].
          ^ #end
        )  )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_CONSTANT_1, BC_POP,
           BC_PUSH_CONSTANT_2, BC_POP, BC_PUSH_0,

           // jump offset, to jump to the pop BC after the if/right before the
           // push #end
           BC(BC_JUMP_ON_FALSE_TOP_NIL, 27, 0), BC(BC_PUSH_CONSTANT, 3), BC_POP,
           BC(BC_PUSH_CONSTANT, 4), BC_POP, BC(BC_PUSH_CONSTANT, 5),

           // jump offset, jump to push_nil as result of whileTrue
           BC(BC_JUMP_ON_FALSE_POP, 15, 0), BC(BC_PUSH_CONSTANT, 6), BC_POP,
           BC(BC_PUSH_CONSTANT, 7), BC_POP, BC(BC_PUSH_CONSTANT, 8), BC_POP,

           // jump offset, jump back to the first PUSH_CONST inside if body,
           // pushing #const3
           BC(BC_JUMP_BACKWARD, 20, 0), BC_PUSH_NIL, BC_POP,

           BC(BC_PUSH_CONSTANT, 9), BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testInliningWhileLoopsWithContractingBranches() {
    auto bytecodes = methodToBytecode(R"""(
                        test = (
                        0 ifTrue: [
                            [ ^ 1 ]
                                whileTrue: [
                                ^ 0 ]
                        ].
                        ^ #end
                        ) )""");

    check(bytecodes,
          {BC_PUSH_0,
           // jump offset to jump to the pop after the if, before pushing #end
           BC(BC_JUMP_ON_FALSE_TOP_NIL, 15, 0), BC_PUSH_1, BC_RETURN_LOCAL,
           // jump offset, jump to push_nil as result of whileTrue
           BC(BC_JUMP_ON_FALSE_POP, 9, 0), BC_PUSH_0, BC_RETURN_LOCAL, BC_POP,
           // jump offset, to the push_1 of the condition
           BC(BC_JUMP_BACKWARD, 8, 0), BC_PUSH_NIL, BC_POP, BC_PUSH_CONSTANT_0,
           BC_RETURN_LOCAL});
};

void BytecodeGenerationTest::testIfInlineAndConstantBcLength() {
    auto bytecodes = methodToBytecode(R"""(
                                test = (
                                  #a. #b. #c.
                                  true ifTrue: [
                                    true ifFalse: [ #e. #f. #g ] ]
                                ) )""");

    CPPUNIT_ASSERT_EQUAL((uint8_t)BC_JUMP_ON_TRUE_TOP_NIL, bytecodes.at(13));

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "jump offset, should point to correct bytecode and not be affected by "
        "changing length of bytecodes in the block",
        (uint8_t)11, bytecodes.at(14));
}

void BytecodeGenerationTest::testIfTrueWithLiteralReturn() {
    ifTrueWithLiteralReturn("0", BC_PUSH_0);
    ifTrueWithLiteralReturn("1", BC_PUSH_1);
    ifTrueWithLiteralReturn("-10", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("3333", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("'str'", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("#sym", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("1.1", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("-2342.234", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("true", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("false", BC_PUSH_CONSTANT_1);
    ifTrueWithLiteralReturn("nil", BC_PUSH_NIL);
    ifTrueWithLiteralReturn("SomeGlobal", BC(BC_PUSH_GLOBAL, 1));
    ifTrueWithLiteralReturn("[]", BC(BC_PUSH_BLOCK_WITHOUT_CONTEXT, 1));
    ifTrueWithLiteralReturn("[ self ]", BC(BC_PUSH_BLOCK, 1));
}

void BytecodeGenerationTest::ifTrueWithLiteralReturn(std::string literal,
                                                     BC bytecode) {
    std::string source = R"""(      test = (
                                        self method ifTrue: [ LITERAL ].
                                    ) )""";
    bool const wasReplaced = ReplacePattern(source, "LITERAL", literal);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());

    bool const twoByte2 = bytecode.bytecode == BC_PUSH_GLOBAL ||
                          bytecode.bytecode == BC_PUSH_BLOCK ||
                          bytecode.bytecode == BC_PUSH_BLOCK_WITHOUT_CONTEXT;

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND_1, 0),
           BC(BC_JUMP_ON_FALSE_TOP_NIL, twoByte2 ? 5 : 4, 0), bytecode, BC_POP,
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIfTrueWithSomethingAndLiteralReturn() {
    ifTrueWithSomethingAndLiteralReturn("0", BC_PUSH_0);
    ifTrueWithSomethingAndLiteralReturn("1", BC_PUSH_1);
    ifTrueWithSomethingAndLiteralReturn("-10", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("3333", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("'str'", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("#sym", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("1.1", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("-2342.234", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("true", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("false", BC_PUSH_CONSTANT_2);
    ifTrueWithSomethingAndLiteralReturn("nil", BC_PUSH_NIL);
    ifTrueWithSomethingAndLiteralReturn("SomeGlobal", BC(BC_PUSH_GLOBAL, 2));
    ifTrueWithSomethingAndLiteralReturn("[]",
                                        BC(BC_PUSH_BLOCK_WITHOUT_CONTEXT, 2));
    ifTrueWithSomethingAndLiteralReturn("[ self ]", BC(BC_PUSH_BLOCK, 2));
}

void BytecodeGenerationTest::ifTrueWithSomethingAndLiteralReturn(
    std::string literal, BC bytecode) {
    // This test is different from the previous one, because the block
    // method won't be recognized as being trivial

    std::string source = R"""(      test = (
                                        self method ifTrue: [ #fooBarNonTrivialBlock. LITERAL ].
                                    ) )""";
    bool const wasReplaced = ReplacePattern(source, "LITERAL", literal);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());

    bool const twoByte2 = bytecode.bytecode == BC_PUSH_GLOBAL ||
                          bytecode.bytecode == BC_PUSH_BLOCK ||
                          bytecode.bytecode == BC_PUSH_BLOCK_WITHOUT_CONTEXT;

    check(bytecodes,
          {BC_PUSH_SELF, BC(BC_SEND_1, 0),
           BC(BC_JUMP_ON_FALSE_TOP_NIL, twoByte2 ? 7 : 6, 0),
           BC_PUSH_CONSTANT_1, BC_POP, bytecode, BC_POP, BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIfTrueIfFalseArg() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ arg1 ] ifFalse: [ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND_1, 1),
           BC(BC_JUMP_ON_FALSE_POP, 7, 0), BC_PUSH_ARG_1, BC(BC_JUMP, 4, 0),
           BC_PUSH_ARG_2, BC_POP, BC_PUSH_CONSTANT_2, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfTrueIfFalseNlrArg1() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ ^ arg1 ] ifFalse: [ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND_1, 1),
           BC(BC_JUMP_ON_FALSE_POP, 8, 0), BC_PUSH_ARG_1, BC_RETURN_LOCAL,
           BC(BC_JUMP, 4, 0), BC_PUSH_ARG_2, BC_POP, BC_PUSH_CONSTANT_2,
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfTrueIfFalseNlrArg2() {
    auto bytecodes = methodToBytecode(R"""( test: arg1 with: arg2 = (
            #start.
            self method ifTrue: [ arg1 ] ifFalse: [ ^ arg2 ].
            #end
        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND_1, 1),
           BC(BC_JUMP_ON_FALSE_POP, 7, 0), BC_PUSH_ARG_1, BC(BC_JUMP, 5, 0),
           BC_PUSH_ARG_2, BC_RETURN_LOCAL, BC_POP, BC_PUSH_CONSTANT_2,
           BC_RETURN_SELF});
}
void BytecodeGenerationTest::testInliningOfOr() {
    inliningOfOr("or:");
    inliningOfOr("||");
}

void BytecodeGenerationTest::inliningOfOr(std::string selector) {
    std::string source = "test = ( true OR_SEL [ #val ] )";
    bool const wasReplaced = ReplacePattern(source, "OR_SEL", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_TRUE_POP, 7, 0),
           // true branch
           BC_PUSH_CONSTANT_1,  // push the `#val`
           BC(BC_JUMP, 4, 0),
           // false branch, jump_on_true target, push true
           BC_PUSH_CONSTANT_0,
           // target of the jump in the true branch
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testInliningOfAnd() {
    inliningOfAnd("and:");
    inliningOfAnd("&&");
}

void BytecodeGenerationTest::inliningOfAnd(std::string selector) {
    std::string source = "test = ( true AND_SEL [ #val ] )";
    bool const wasReplaced = ReplacePattern(source, "AND_SEL", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_FALSE_POP, 7, 0),
           // true branch
           BC_PUSH_CONSTANT_1,  // push the `#val`
           BC(BC_JUMP, 4, 0),
           // false branch, jump_on_false target, push false
           BC_PUSH_CONSTANT_2,
           // target of the jump in the true branch
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testInliningOfToDo() {
    auto bytecodes = methodToBytecode("test = ( 1 to: 2 do: [:i | i ] )");
    check(bytecodes,
          {BC_PUSH_1, BC_PUSH_CONSTANT_0,
           BC_DUP_SECOND,  // stack: Top[1, 2, 1]

           BC(BC_JUMP_IF_GREATER, 11, 0),  // consume only on jump
           BC_DUP,

           BC_POP_LOCAL_0,   // store the i into the local (arg becomes local
                             // after inlining)
           BC_PUSH_LOCAL_0,  // push the local on the stack as part of the
                             // block's code
           BC_POP,           // cleanup after block
           BC_INC,           // increment top, the iteration counter

           // jump back to the jump_if_greater bytecode
           BC(BC_JUMP_BACKWARD, 8, 0),

           // jump_if_greater target
           BC_RETURN_SELF});
}

static std::vector<uint8_t> GetBytecodes(VMMethod* method) {
    std::vector<uint8_t> bcs(method->bcLength);

    for (size_t i = 0; i < method->bcLength; i += 1) {
        bcs.at(i) = method->GetBytecode(i);
    }
    return bcs;
}

void BytecodeGenerationTest::testToDoBlockBlockInlinedSelf() {
    auto bytecodes = methodToBytecode(R"""(
                                      test = (
                                        | l1 l2 |
                                        1 to: 2 do: [:a |
                                            l1 do: [:b |
                                            b ifTrue: [
                                                a.
                                                l2 := l2 + 1 ] ] ]
                                      ) )""");
    check(bytecodes,
          {BC_PUSH_1, BC_PUSH_CONSTANT_0,
           BC_DUP_SECOND,  // stack: Top[1, 2, 1]

           BC(BC_JUMP_IF_GREATER, 15, 0),  // consume only on jump
           BC_DUP,

           BC_POP_LOCAL_2,                        // store the `a`
           BC_PUSH_LOCAL_0,                       // push the `l1` on the stack
           BC(BC_PUSH_BLOCK, 1), BC(BC_SEND, 2),  // send #do:
           BC_POP,
           BC_INC,  // increment top, the iteration counter

           // jump back to the jump_if_greater bytecode
           BC(BC_JUMP_BACKWARD, 12, 0),

           // jump_if_greater target
           BC_RETURN_SELF});

    auto* block = (VMMethod*)_mgenc->GetLiteral(1);
    check(GetBytecodes(block),
          {BC_PUSH_ARG_1, BC(BC_JUMP_ON_FALSE_TOP_NIL, 15, 0),
           BC(BC_PUSH_LOCAL, 2, 1),  // load the `a`
           BC_POP, BC(BC_PUSH_LOCAL, 1, 1), BC_INC, BC_DUP,
           BC(BC_POP_LOCAL, 1, 1), BC_RETURN_LOCAL},
          block);
}

void BytecodeGenerationTest::testToDoWithMoreEmbeddedBlocksAndArgAccess() {
    auto bytecodes = methodToBytecode(R"""(
                                        transferEntries: oldStorage = (
                                          1 to: oldStorage length do: [:i |
                                            | current |
                                            current := oldStorage at: i.
                                            current notInlined: [
                                              oldStorage at: i put: nil.
                                              current next
                                                noInline: [ i. current. #foo ]
                                                noInline: [
                                                  self splitBucket: oldStorage bucket: i head: current ] ] ]
                                        ) )""");
    check(bytecodes,
          {BC_PUSH_1, BC_PUSH_ARG_1, BC(BC_SEND_1, 0),
           BC_DUP_SECOND,  //~

           BC(BC_JUMP_IF_GREATER, 20, 0),  // consume only on jump
           BC_DUP,

           BC_POP_LOCAL_0,   // i
           BC_PUSH_ARG_1,    // oldStorage
           BC_PUSH_LOCAL_0,  // i
           BC(BC_SEND, 1),   // #at:

           BC_POP_LOCAL_1,        // current
           BC_PUSH_LOCAL_1,       // current
           BC(BC_PUSH_BLOCK, 2),  // ~
           BC(BC_SEND, 3),        // send #notInlined:
           BC_POP,
           BC_INC,  // increment top, the iteration counter

           // jump back to the jump_if_greater bytecode
           BC(BC_JUMP_BACKWARD, 17, 0),

           // jump_if_greater target
           BC_RETURN_SELF});

    auto* block = (VMMethod*)_mgenc->GetLiteral(2);
    check(GetBytecodes(block),
          {BC(BC_PUSH_ARGUMENT, 1, 1),   // oldStorage
           BC(BC_PUSH_LOCAL, 0, 1),      // i
           BC_PUSH_NIL, BC(BC_SEND, 0),  // #at:put:
           BC_POP,

           BC(BC_PUSH_LOCAL, 1, 1),  // current
           BC(BC_SEND_1, 1),         // #next
           BC(BC_PUSH_BLOCK, 2),     //~
           BC(BC_PUSH_BLOCK, 3),     //~
           BC(BC_SEND, 4),           // #noInline:noInline:
           BC_RETURN_LOCAL},
          block);

    // [ i. current. #foo ]
    auto* block2 = (VMMethod*)block->GetIndexableField(2);
    check(GetBytecodes(block2),
          {BC(BC_PUSH_LOCAL, 0, 2),  // i
           BC_POP,                   //~
           BC(BC_PUSH_LOCAL, 1, 2),  // current
           BC_POP,                   //~
           BC_PUSH_CONSTANT_0, BC_RETURN_LOCAL},
          block2);

    // [ self splitBucket: oldStorage bucket: i head: current ]
    auto* block3 = (VMMethod*)block->GetIndexableField(3);
    check(GetBytecodes(block3),
          {BC(BC_PUSH_ARGUMENT, 0, 2),  // self
           BC(BC_PUSH_ARGUMENT, 1, 2),  // oldStorage
           BC(BC_PUSH_LOCAL, 0, 2),     // i
           BC(BC_PUSH_LOCAL, 1, 2),     // current
           BC(BC_SEND, 0),              // #splitBucket:bucket:head:
           BC_RETURN_LOCAL},
          block3);
}

void BytecodeGenerationTest::testIfArg() {
    ifArg("ifTrue:", BC_JUMP_ON_FALSE_TOP_NIL);
    ifArg("ifFalse:", BC_JUMP_ON_TRUE_TOP_NIL);
    ifArg("ifNil:", BC_JUMP_ON_NOT_NIL_TOP_TOP);
    ifArg("ifNotNil:", BC_JUMP_ON_NIL_TOP_TOP);
}

void BytecodeGenerationTest::ifArg(std::string selector, int8_t jumpBytecode) {
    std::string source = R"""(      test: arg = (
                                         #start.
                                         self method IF_SELECTOR [ arg ].
                                         #end
                                     ) )""";
    bool const wasReplaced = ReplacePattern(source, "IF_SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND_1, 1),
           BC(jumpBytecode, 4, 0), BC_PUSH_ARG_1, BC_POP, BC_PUSH_CONSTANT_2,
           BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testKeywordIfTrueArg() {
    auto bytecodes = methodToBytecode(R"""(      test: arg = (
                                                     #start.
                                                     (self key: 5) ifTrue: [ arg ].
                                                     #end
                                                 ) )""");
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC_PUSH_CONSTANT_1,
           BC(BC_SEND, 2), BC(BC_JUMP_ON_FALSE_TOP_NIL, 4, 0), BC_PUSH_ARG_1,
           BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfReturnNonLocal() {
    ifReturnNonLocal("ifTrue:", BC_JUMP_ON_FALSE_TOP_NIL);
    ifReturnNonLocal("ifFalse:", BC_JUMP_ON_TRUE_TOP_NIL);
}

void BytecodeGenerationTest::ifReturnNonLocal(std::string selector,
                                              int8_t jumpBytecode) {
    std::string source = R"""(      test: arg = (
                                         #start.
                                         self method IF_SELECTOR [ ^ arg ].
                                         #end
                                     ) )""";
    bool const wasReplaced = ReplacePattern(source, "IF_SELECTOR", selector);
    assert(wasReplaced);

    auto bytecodes = methodToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC(BC_SEND_1, 1),
           BC(jumpBytecode, 5, 0), BC_PUSH_ARG_1, BC_RETURN_LOCAL, BC_POP,
           BC_PUSH_CONSTANT_2, BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testNestedIfs() {
    addField("field");
    auto bytecodes = methodToBytecode(R"""(      test: arg = (
                                                     true ifTrue: [
                                                         false ifFalse: [
                                                           ^ field - arg
                                                         ]
                                                     ]
                                                 ) )""");
    check(
        bytecodes,
        {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_FALSE_TOP_NIL, 12, 0),
         BC_PUSH_CONSTANT_1, BC(BC_JUMP_ON_TRUE_TOP_NIL, 8, 0), BC_PUSH_FIELD_0,
         BC_PUSH_ARG_1, BC(BC_SEND, 2), BC_RETURN_LOCAL, BC_RETURN_SELF});
}

void BytecodeGenerationTest::testNestedIfsAndLocals() {
    addField("field");
    auto bytecodes = methodToBytecode(R"""(
            test: arg = (
               | a b c d |
               a := b.
               true ifTrue: [
                 | e f g |
                 e := 2.
                 c := 3.
                 false ifFalse: [
                   | h i j |
                   h := 1.
                   ^ i - j - f - g - d ] ] )
        )""");
    check(bytecodes, {BC_PUSH_LOCAL_1,
                      BC_POP_LOCAL_0,
                      BC_PUSH_CONSTANT_0,
                      BC(BC_JUMP_ON_FALSE_TOP_NIL, 42, 0),
                      BC_PUSH_CONSTANT_1,
                      BC(BC_POP_LOCAL, 4, 0),
                      BC_PUSH_CONSTANT_2,
                      BC_POP_LOCAL_2,
                      BC(BC_PUSH_CONSTANT, 3),
                      BC(BC_JUMP_ON_TRUE_TOP_NIL, 31, 0),
                      BC_PUSH_1,
                      BC(BC_POP_LOCAL, 7, 0),
                      BC(BC_PUSH_LOCAL, 8, 0),
                      BC(BC_PUSH_LOCAL, 9, 0),
                      BC(BC_SEND, 4),
                      BC(BC_PUSH_LOCAL, 5, 0),
                      BC(BC_SEND, 4),
                      BC(BC_PUSH_LOCAL, 6, 0),
                      BC(BC_SEND, 4),
                      BC(BC_PUSH_LOCAL, 3, 0),
                      BC(BC_SEND, 4),
                      BC_RETURN_LOCAL,
                      BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIncDecBytecodes() {
    incDecBytecodes("+", BC_INC);
    incDecBytecodes("-", BC_DEC);
}

void BytecodeGenerationTest::incDecBytecodes(const std::string& sel,
                                             uint8_t bc) {
    std::string source = "test = ( 1 " + sel + " 1 )";
    auto bytecodes = methodToBytecode(source.data());

    check(bytecodes, {BC_PUSH_1, bc, BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIfTrueAndIncField() {
    addField("field");

    auto bytecodes = methodToBytecode(R"""(
                        test: arg = (
                            #start.
                            (self key: 5) ifTrue: [ field := field + 1 ].
                            #end
                        ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC_PUSH_CONSTANT_1,
           BC(BC_SEND, 2), BC(BC_JUMP_ON_FALSE_TOP_NIL, 5, 0),
           BC(BC_INC_FIELD_PUSH, 0), BC_POP, BC(BC_PUSH_CONSTANT, 3),
           BC_RETURN_SELF});
}

void BytecodeGenerationTest::testIfTrueAndIncArg() {
    auto bytecodes = methodToBytecode(R"""(
                                 test: arg = (
                                     #start.
                                     (self key: 5) ifTrue: [ arg + 1 ].
                                     #end
                                 ) )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF, BC_PUSH_CONSTANT_1,
           BC(BC_SEND, 2), BC(BC_JUMP_ON_FALSE_TOP_NIL, 5, 0), BC_PUSH_ARG_1,
           BC_INC, BC_POP, BC(BC_PUSH_CONSTANT, 3), BC_RETURN_SELF});
}

/*



 def test_nested_ifs_and_non_inlined_blocks(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: arg = (
           | a |
           a := 1.
           true ifTrue: [
             | e |
             e := 0.
             [ a := 1. a ].
             false ifFalse: [
               | h |
               h := 1.
               [ h + a + e ].
               ^ h ] ].

           [ a ]
         )""",
     )

     assert len(bytecodes) == 35
     check(
         bytecodes,
         [
             (4, Bytecodes.push_constant_1),
             BC(Bytecodes.jump_on_false_top_nil, 26),
             (9, BC(Bytecodes.pop_local, 1, 0, note="local e")),
             (17, BC(Bytecodes.jump_on_true_top_nil, 14)),
             (21, BC(Bytecodes.pop_local, 2, 0, note="local h")),
         ],
     )

     check(
         mgenc.get_constant(12).get_bytecodes(),
         [(1, BC(Bytecodes.pop_local, 0, 1, note="local a"))],
     )

     check(
         mgenc.get_constant(24).get_bytecodes(),
         [
             BC(Bytecodes.push_local, 2, 1, note="local h"),
             BC(Bytecodes.push_local, 0, 1, note="local a"),
             (8, BC(Bytecodes.push_local, 1, 1, note="local e")),
         ],
     )

     check(
         mgenc.get_constant(32).get_bytecodes(),
         [BC(Bytecodes.push_local, 0, 1, note="local a")],
     )


 def test_nested_non_inlined_blocks(cgenc, mgenc):
     add_field(cgenc, "field")
     bytecodes = method_to_bytecodes(
         mgenc,
         """
         test: a = ( | b |
           true ifFalse: [ | c |
             a. b. c.
             [:d |
               a. b. c. d.
               [:e |
                 a. b. c. d. e ] ] ]
         )""",
     )

     assert len(bytecodes) == 19
     check(
         bytecodes,
         [
             (1, BC(Bytecodes.jump_on_true_top_nil, 17)),
             BC(Bytecodes.push_argument, 1, 0, note="arg a"),
             (8, BC(Bytecodes.push_local, 0, 0, note="local b")),
             (12, BC(Bytecodes.push_local, 1, 0, note="local c")),
         ],
     )

     block_method = mgenc.get_constant(16)
     check(
         block_method.get_bytecodes(),
         [
             BC(Bytecodes.push_argument, 1, 1, note="arg a"),
             (4, BC(Bytecodes.push_local, 0, 1, note="local b")),
             (8, BC(Bytecodes.push_local, 1, 1, note="local c")),
             (12, BC(Bytecodes.push_argument, 1, 0, note="arg d")),
         ],
     )

     block_method = block_method.get_constant(16)
     check(
         block_method.get_bytecodes(),
         [
             BC(Bytecodes.push_argument, 1, 2, note="arg a"),
             (4, BC(Bytecodes.push_local, 0, 2, note="local b")),
             (8, BC(Bytecodes.push_local, 1, 2, note="local c")),
             (12, BC(Bytecodes.push_argument, 1, 1, note="arg d")),
             (16, BC(Bytecodes.push_argument, 1, 0, note="arg e")),
         ],
     )
*/

void BytecodeGenerationTest::testBlockIfTrueArg() {
    auto bytecodes = blockToBytecode(R"""(
               [:arg | #start.
                   self method ifTrue: [ arg ].
                   #end
               ] )""");

    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC(BC_PUSH_ARGUMENT, 0, 1),
           BC(BC_SEND_1, 1), BC(BC_JUMP_ON_FALSE_TOP_NIL, 4, 0), BC_PUSH_ARG_1,
           BC_POP, BC_PUSH_CONSTANT_2, BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testBlockIfTrueMethodArg() {
    ensureMGenC();
    std::string argName = "arg";
    _mgenc->AddArgument(argName, {1, 1}, nullptr);

    auto bytecodes = blockToBytecode(R"""(
               [ #start.
                   self method ifTrue: [ arg ].
                   #end
               ] )""");

    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP, BC(BC_PUSH_ARGUMENT, 0, 1),
                      BC(BC_SEND_1, 1), BC(BC_JUMP_ON_FALSE_TOP_NIL, 6, 0),
                      BC(BC_PUSH_ARGUMENT, 1, 1), BC_POP, BC_PUSH_CONSTANT_2,
                      BC_RETURN_LOCAL});
}

void BytecodeGenerationTest::testIfTrueIfFalseReturn() {
    ifTrueIfFalseReturn("ifTrue:", "ifFalse:", BC(BC_JUMP_ON_FALSE_POP, 8, 0));
    ifTrueIfFalseReturn("ifFalse:", "ifTrue:", BC(BC_JUMP_ON_TRUE_POP, 8, 0));

    ifTrueIfFalseReturn(
        "ifNil:", "ifNotNil:", BC(BC_JUMP_ON_NOT_NIL_POP, 8, 0));
    ifTrueIfFalseReturn("ifNotNil:", "ifNil:", BC(BC_JUMP_ON_NIL_POP, 8, 0));
}
void BytecodeGenerationTest::ifTrueIfFalseReturn(const std::string& sel1,
                                                 const std::string& sel2,
                                                 BC bc) {
    std::string source = "test: arg1 with: arg2 = ( #start. ^ self method " +
                         sel1 + " [ ^ arg1 ] " + sel2 + " [ arg2 ] )";
    auto bytecodes = methodToBytecode(source.data());

    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP, BC_PUSH_SELF,
                      BC(BC_SEND_1, 1), bc, BC_PUSH_ARG_1, BC_RETURN_LOCAL,
                      BC(BC_JUMP, 4, 0), BC_PUSH_ARG_2, BC_RETURN_LOCAL});
    tearDown();
}

void BytecodeGenerationTest::testBlockIfReturnNonLocal() {
    blockIfReturnNonLocal("ifTrue:", BC(BC_JUMP_ON_FALSE_TOP_NIL, 5, 0));
    blockIfReturnNonLocal("ifFalse:", BC(BC_JUMP_ON_TRUE_TOP_NIL, 5, 0));
}

void BytecodeGenerationTest::blockIfReturnNonLocal(std::string sel, BC bc) {
    std::string source = R"""(      [:arg |
             #start.
             self method IF_SELECTOR [ ^ arg ].
             #end
         ] )""";
    bool const wasReplaced = ReplacePattern(source, "IF_SELECTOR", sel);
    assert(wasReplaced);

    auto bytecodes = blockToBytecode(source.data());
    check(bytecodes,
          {BC_PUSH_CONSTANT_0, BC_POP, BC(BC_PUSH_ARGUMENT, 0, 1),
           BC(BC_SEND_1, 1), bc, BC_PUSH_ARG_1, BC_RETURN_NON_LOCAL, BC_POP,
           BC_PUSH_CONSTANT_2, BC_RETURN_LOCAL});

    tearDown();
}

void BytecodeGenerationTest::testTrivialMethodInlining() {
    trivialMethodInlining("0", BC_PUSH_0);
    trivialMethodInlining("1", BC_PUSH_1);
    trivialMethodInlining("-10", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("3333", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("'str'", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("#sym", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("1.1", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("-2342.234", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("true", BC_PUSH_CONSTANT_0);
    trivialMethodInlining("false", BC_PUSH_CONSTANT_1);
    trivialMethodInlining("nil", BC_PUSH_NIL);
    trivialMethodInlining("Nil", BC(BC_PUSH_GLOBAL, 1));
    trivialMethodInlining("UnknownGlobal", BC(BC_PUSH_GLOBAL, 1));
    trivialMethodInlining("[]", BC(BC_PUSH_BLOCK_WITHOUT_CONTEXT, 1));
    trivialMethodInlining("[ self ]", BC(BC_PUSH_BLOCK, 1));
}

void BytecodeGenerationTest::trivialMethodInlining(const std::string& literal,
                                                   BC bytecode) {
    std::string source = "test = ( true ifTrue: [ " + literal + " ] )";
    auto bytecodes = methodToBytecode(source.data());

    bool const isLongerBytecode =
        bytecode.bytecode == BC_PUSH_GLOBAL ||
        bytecode.bytecode == BC_PUSH_BLOCK ||
        bytecode.bytecode == BC_PUSH_BLOCK_WITHOUT_CONTEXT;
    check(bytecodes, {BC_PUSH_CONSTANT_0,
                      BC(BC_JUMP_ON_FALSE_TOP_NIL, isLongerBytecode ? 5 : 4, 0),
                      bytecode, BC_RETURN_SELF});
    tearDown();
}

void BytecodeGenerationTest::testIncField() {
    incField(0);
    incField(1);
    incField(2);
    incField(3);
    incField(4);
    incField(5);
    incField(6);
}

void BytecodeGenerationTest::incField(size_t fieldNum) {
    addField("field0");
    addField("field1");
    addField("field2");
    addField("field3");
    addField("field4");
    addField("field5");
    addField("field6");

    std::string const fieldName = "field" + to_string(fieldNum);
    std::string source =
        "test = ( " + fieldName + " := " + fieldName + " + 1 )";

    auto bytecodes = methodToBytecode(source.data());

    check(bytecodes, {BC(BC_INC_FIELD, fieldNum), BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testIncFieldNonTrivial() {
    incFieldNonTrivial(0);
    incFieldNonTrivial(1);
    incFieldNonTrivial(2);
    incFieldNonTrivial(3);
    incFieldNonTrivial(4);
    incFieldNonTrivial(5);
    incFieldNonTrivial(6);
}

void BytecodeGenerationTest::incFieldNonTrivial(size_t fieldNum) {
    addField("field0");
    addField("field1");
    addField("field2");
    addField("field3");
    addField("field4");
    addField("field5");
    addField("field6");

    std::string const fieldName = "field" + to_string(fieldNum);
    std::string source =
        "test = ( 1. " + fieldName + " := " + fieldName + " + 1. 2 )";

    auto bytecodes = methodToBytecode(source.data());

    check(bytecodes, {BC_PUSH_1, BC_POP, BC(BC_INC_FIELD, fieldNum),
                      BC_PUSH_CONSTANT_0, BC_RETURN_SELF});

    tearDown();
}

void BytecodeGenerationTest::testReturnIncField() {
    returnIncField(0);
    returnIncField(1);
    returnIncField(2);
    returnIncField(3);
    returnIncField(4);
    returnIncField(5);
    returnIncField(6);
}

void BytecodeGenerationTest::returnIncField(size_t fieldNum) {
    addField("field0");
    addField("field1");
    addField("field2");
    addField("field3");
    addField("field4");
    addField("field5");
    addField("field6");

    std::string const fieldName = "field" + to_string(fieldNum);
    std::string source =
        "test = ( #foo. ^ " + fieldName + " := " + fieldName + " + 1 )";

    auto bytecodes = methodToBytecode(source.data());

    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP,
                      BC(BC_INC_FIELD_PUSH, fieldNum), BC_RETURN_LOCAL});

    tearDown();
}

void BytecodeGenerationTest::testReturnIncFieldFromBlock() {
    returnIncFieldFromBlock(0);
    returnIncFieldFromBlock(1);
    returnIncFieldFromBlock(2);
    returnIncFieldFromBlock(3);
    returnIncFieldFromBlock(4);
    returnIncFieldFromBlock(5);
    returnIncFieldFromBlock(6);
}
void BytecodeGenerationTest::returnIncFieldFromBlock(size_t fieldNum) {
    addField("field0");
    addField("field1");
    addField("field2");
    addField("field3");
    addField("field4");
    addField("field5");
    addField("field6");

    std::string const fieldName = "field" + to_string(fieldNum);
    std::string source = "[ #foo. " + fieldName + " := " + fieldName + " + 1 ]";

    auto bytecodes = blockToBytecode(source.data());

    check(bytecodes, {BC_PUSH_CONSTANT_0, BC_POP,
                      BC(BC_INC_FIELD_PUSH, fieldNum), BC_RETURN_LOCAL});

    tearDown();
}

void BytecodeGenerationTest::testReturnField() {
    returnField(0, BC_RETURN_FIELD_0, true);
    returnField(1, BC_RETURN_FIELD_1, true);
    returnField(2, BC_RETURN_FIELD_2, true);
    returnField(3, BC(BC_PUSH_FIELD, 3), false);
    returnField(4, BC(BC_PUSH_FIELD, 4), false);
    returnField(5, BC(BC_PUSH_FIELD, 5), false);
    returnField(6, BC(BC_PUSH_FIELD, 6), false);
}

void BytecodeGenerationTest::returnField(size_t fieldNum, BC bytecode,
                                         bool isReturnFieldBc) {
    addField("field0");
    addField("field1");
    addField("field2");
    addField("field3");
    addField("field4");
    addField("field5");
    addField("field6");

    std::string const fieldName = "field" + to_string(fieldNum);
    std::string source = "test = ( 1. ^ " + fieldName + ")";
    auto bytecodes = methodToBytecode(source.data());
    std::vector<BC> expected = {BC_PUSH_1, BC_POP, bytecode};

    if (!isReturnFieldBc) {
        expected.emplace_back(BC_RETURN_LOCAL);
    }

    check(bytecodes, expected);
    tearDown();
}

void BytecodeGenerationTest::testFieldReadInlining() {
    addField("field");
    auto bytecodes = methodToBytecode("test = ( true and: [ field ] )");
    check(bytecodes, {BC_PUSH_CONSTANT_0, BC(BC_JUMP_ON_FALSE_POP, 7, 0),

                      // true branch
                      BC_PUSH_FIELD_0, BC(BC_JUMP, 4, 0),

                      // false branch, jump_on_true target, push true
                      BC_PUSH_CONSTANT_1,

                      // target of the jump in the true branch
                      BC_RETURN_SELF});
}

void BytecodeGenerationTest::testJumpQueuesOrdering() {
    std::priority_queue<Jump> jumps;

    jumps.emplace(1, BC_JUMP, 0);
    jumps.emplace(5, BC_JUMP, 0);
    jumps.emplace(8, BC_JUMP, 0);
    jumps.emplace(2, BC_JUMP, 0);

    CPPUNIT_ASSERT_EQUAL((size_t)1, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)2, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)5, jumps.top().originalJumpTargetIdx);
    jumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)8, jumps.top().originalJumpTargetIdx);
    jumps.pop();

    std::priority_queue<BackJump> backJumps;
    backJumps.emplace(13, 9);
    backJumps.emplace(3, 12);
    backJumps.emplace(54, 54);

    CPPUNIT_ASSERT_EQUAL((size_t)3, backJumps.top().loopBeginIdx);
    backJumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)13, backJumps.top().loopBeginIdx);
    backJumps.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)54, backJumps.top().loopBeginIdx);
    backJumps.pop();

    std::priority_queue<BackJumpPatch> backJumpsToPatch;
    backJumpsToPatch.emplace(3, 2);
    backJumpsToPatch.emplace(32, 44);
    backJumpsToPatch.emplace(12, 55);

    CPPUNIT_ASSERT_EQUAL((size_t)3, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)12, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
    CPPUNIT_ASSERT_EQUAL((size_t)32, backJumpsToPatch.top().backwardsJumpIdx);
    backJumpsToPatch.pop();
}

void BytecodeGenerationTest::testFieldReadIncWrite() {
    addField("counter");
    auto bytecodes = methodToBytecode(R""""(
        benchmark = ( | iter |
                counter := 0.
                iter := 20000.

                [ iter > 0 ] whileTrue: [
                  iter := iter - 1.
                  counter := counter + 1.
                  counter := counter + 1.
                ].

                ^ counter
            )
        )"""");
    check(bytecodes, {// counter := 0
                      BC_PUSH_0, BC_POP_FIELD_0,

                      // iter := 20000
                      BC_PUSH_CONSTANT_0, BC_POP_LOCAL_0,

                      // iter > 0
                      BC_PUSH_LOCAL_0, BC_PUSH_0, BC(BC_SEND, 1),

                      // whileTrue
                      BC(BC_JUMP_ON_FALSE_POP, 14, 0),

                      // iter := iter - 1
                      BC_PUSH_LOCAL_0, BC_DEC, BC_POP_LOCAL_0,

                      // counter := counter + 1
                      BC(BC_INC_FIELD, 0),

                      // counter := counter + 1
                      BC(BC_INC_FIELD_PUSH, 0),

                      // return to top
                      BC_POP, BC(BC_JUMP_BACKWARD, 15, 0),

                      // end loop
                      BC_PUSH_NIL, BC_POP,

                      // ^ counter
                      BC_RETURN_FIELD_0});
}
