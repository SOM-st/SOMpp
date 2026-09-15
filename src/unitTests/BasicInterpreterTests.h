#pragma once

#include <cppunit/extensions/HelperMacros.h>
#include <utility>

#include "../memory/Heap.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMSymbol.h"

using namespace std;

enum ResultType : uint8_t { INTEGER, CLASS, SYMBOL, DOUBLE };

class TestData {
public:
    std::string className;
    std::string methodName;
    void* expectedResult;
    ResultType type;

    TestData(std::string className, std::string methodName,
             intptr_t expectedResult, ResultType type)
        : className(std::move(className)), methodName(std::move(methodName)),
          expectedResult((void*)expectedResult), type(type) {}

    TestData(std::string className, std::string methodName,
             double const* expectedResult, ResultType type)
        : className(std::move(className)), methodName(std::move(methodName)),
          expectedResult((void*)expectedResult), type(type) {}

    TestData(std::string className, std::string methodName,
             char const* expectedResult, ResultType type)
        : className(std::move(className)), methodName(std::move(methodName)),
          expectedResult((void*)expectedResult), type(type) {}
};

static const double dbl375 = 3.75;

inline std::ostream& operator<<(std::ostream& strm, const TestData& data) {
    strm << data.className << "." << data.methodName;
    return strm;
}

class BasicInterpreterTests : public CPPUNIT_NS::TestFixture {
    // NOLINTNEXTLINE(misc-const-correctness)
    CPPUNIT_TEST_SUITE(BasicInterpreterTests);
    CPPUNIT_TEST_PARAMETERIZED(
        testBasic,
        {
            TestData("MethodCall", "test", 42, INTEGER),
            TestData("MethodCall", "test2", 42, INTEGER),

            TestData("NonLocalReturn", "test1", 42, INTEGER),
            TestData("NonLocalReturn", "test2", 43, INTEGER),
            TestData("NonLocalReturn", "test3", 3, INTEGER),
            TestData("NonLocalReturn", "test4", 42, INTEGER),
            TestData("NonLocalReturn", "test5", 22, INTEGER),

            TestData("Blocks", "testArg1", 42, INTEGER),
            TestData("Blocks", "testArg2", 77, INTEGER),
            TestData("Blocks", "testArgAndLocal", 8, INTEGER),
            TestData("Blocks", "testArgAndContext", 8, INTEGER),
            TestData("Blocks", "testEmptyZeroArg", 1, INTEGER),
            TestData("Blocks", "testEmptyOneArg", 1, INTEGER),
            TestData("Blocks", "testEmptyTwoArg", 1, INTEGER),

            TestData("Return", "testReturnSelf", "Return", CLASS),
            TestData("Return", "testReturnSelfImplicitly", "Return", CLASS),
            TestData("Return", "testNoReturnReturnsSelf", "Return", CLASS),
            TestData("Return", "testBlockReturnsImplicitlyLastValue", 4,
                     INTEGER),

            TestData("IfTrueIfFalse", "test", 42, INTEGER),
            TestData("IfTrueIfFalse", "test2", 33, INTEGER),
            TestData("IfTrueIfFalse", "test3", 4, INTEGER),

            TestData("IfTrueIfFalse", "testIfTrueTrueResult", "Integer", CLASS),
            TestData("IfTrueIfFalse", "testIfTrueFalseResult", "Nil", CLASS),
            TestData("IfTrueIfFalse", "testIfFalseTrueResult", "Nil", CLASS),
            TestData("IfTrueIfFalse", "testIfFalseFalseResult", "Integer",
                     CLASS),

            TestData("CompilerSimplification", "testReturnConstantSymbol",
                     "constant", SYMBOL),
            TestData("CompilerSimplification", "testReturnConstantInt", 42,
                     INTEGER),
            TestData("CompilerSimplification", "testReturnSelf",
                     "CompilerSimplification", CLASS),
            TestData("CompilerSimplification", "testReturnSelfImplicitly",
                     "CompilerSimplification", CLASS),
            TestData("CompilerSimplification", "testReturnArgumentN", 55,
                     INTEGER),
            TestData("CompilerSimplification", "testReturnArgumentA", 44,
                     INTEGER),
            TestData("CompilerSimplification", "testSetField", "foo", SYMBOL),
            TestData("CompilerSimplification", "testGetField", 40, INTEGER),

            TestData("Hash", "testHash", 444, INTEGER),

            TestData("Arrays", "testEmptyToInts", 3, INTEGER),
            TestData("Arrays", "testPutAllInt", 5, INTEGER),
            TestData("Arrays", "testPutAllNil", "Nil", CLASS),
            TestData("Arrays", "testPutAllBlock", 3, INTEGER),
            TestData("Arrays", "testNewWithAll", 1, INTEGER),

            TestData("BlockInlining", "testNoInlining", 1, INTEGER),
            TestData("BlockInlining", "testOneLevelInlining", 1, INTEGER),
            TestData("BlockInlining", "testOneLevelInliningWithLocalShadowTrue",
                     2, INTEGER),
            TestData("BlockInlining",
                     "testOneLevelInliningWithLocalShadowFalse", 1, INTEGER),

            TestData("BlockInlining", "testShadowDoesntStoreWrongLocal", 33,
                     INTEGER),
            TestData("BlockInlining", "testShadowDoesntReadUnrelated", "Nil",
                     CLASS),

            TestData("BlockInlining", "testBlockNestedInIfTrue", 2, INTEGER),
            TestData("BlockInlining", "testBlockNestedInIfFalse", 42, INTEGER),

            TestData("BlockInlining", "testStackDisciplineTrue", 1, INTEGER),
            TestData("BlockInlining", "testStackDisciplineFalse", 2, INTEGER),

            TestData("BlockInlining", "testDeepNestedInlinedIfTrue", 3,
                     INTEGER),
            TestData("BlockInlining", "testDeepNestedInlinedIfFalse", 42,
                     INTEGER),

            TestData("BlockInlining", "testDeepNestedBlocksInInlinedIfTrue", 5,
                     INTEGER),
            TestData("BlockInlining", "testDeepNestedBlocksInInlinedIfFalse",
                     43, INTEGER),

            TestData("BlockInlining", "testDeepDeepNestedTrue", 9, INTEGER),
            TestData("BlockInlining", "testDeepDeepNestedFalse", 43, INTEGER),

            TestData("BlockInlining", "testToDoNestDoNestIfTrue", 2, INTEGER),

            TestData("NonLocalVars", "testWriteDifferentTypes", &dbl375,
                     DOUBLE),

            TestData("ObjectCreation", "test", 1000000, INTEGER),

            TestData("Regressions", "testSymbolEquality", 1, INTEGER),
            TestData("Regressions", "testSymbolReferenceEquality", 1, INTEGER),
            TestData("Regressions", "testUninitializedLocal", 1, INTEGER),
            TestData("Regressions", "testUninitializedLocalInBlock", 1,
                     INTEGER),

            TestData("BinaryOperation", "test", 11, INTEGER),

            TestData("NumberOfTests", "numberOfTests", 65, INTEGER),
        });

    CPPUNIT_TEST_SUITE_END();

private:
    static void assertEqualsSOMValue(vm_oop_t actualResult, TestData& data) {
        if (data.type == INTEGER) {
            auto const expected = (int64_t)(intptr_t)data.expectedResult;
            int64_t const actual = SMALL_INT_VAL(actualResult);
            CPPUNIT_ASSERT_EQUAL(expected, actual);
            return;
        }

        if (data.type == DOUBLE) {
            auto* expected = (double*)data.expectedResult;
            double const actual =
                ((VMDouble*)actualResult)->GetEmbeddedDouble();
            CPPUNIT_ASSERT_DOUBLES_EQUAL(*expected, actual, 1e-15);
            return;
        }

        if (data.type == CLASS) {
            char const* expected = (char const*)data.expectedResult;
            std::string const actual =
                ((VMClass*)actualResult)->GetName()->GetStdString();
            if (expected != actual) {
                CPPUNIT_NS::Asserter::failNotEqual(expected, actual,
                                                   CPPUNIT_SOURCELINE());
            }
            return;
        }

        if (data.type == SYMBOL) {
            char const* expected = (char const*)data.expectedResult;
            std::string const actual =
                ((VMSymbol*)actualResult)->GetStdString();
            if (expected != actual) {
                CPPUNIT_NS::Asserter::failNotEqual(expected, actual,
                                                   CPPUNIT_SOURCELINE());
            }
            return;
        }

        CPPUNIT_FAIL("SOM Value handler missing");
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void testBasic(TestData data) {
        bool const oldGcStressMode = gcStressMode;
        if (data.className == "ObjectCreation") {
            gcStressMode = false;  // it's too slow...
        }

        // The Unit Test harness will initialize Universe for a standard run.
        // This is different from other SOMs.
        vm_oop_t result = Universe::interpret(data.className, data.methodName);

        if (data.className == "ObjectCreation") {
            gcStressMode = oldGcStressMode;
        }

        CPPUNIT_ASSERT(result != nullptr);
        assertEqualsSOMValue(result, data);
    }
};
