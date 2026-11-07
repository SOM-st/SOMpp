#pragma once
/*
 * WalkObjectsTest.h
 *
 *  Created on: 13.01.2011
 *      Author: christian
 */

#include <cppunit/extensions/HelperMacros.h>

#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"

using namespace std;

class WalkObjectsTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(WalkObjectsTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testWalkArray);
    CPPUNIT_TEST(testWalkBlock);
    CPPUNIT_TEST(testWalkClass);
    CPPUNIT_TEST(testWalkDouble);
    CPPUNIT_TEST(testWalkEvaluationPrimitive);
    CPPUNIT_TEST(testWalkFrame);
    CPPUNIT_TEST(testWalkInteger);
    CPPUNIT_TEST(testWalkString);
    CPPUNIT_TEST(testWalkMethod);
    CPPUNIT_TEST(testWalkObject);
    CPPUNIT_TEST(testWalkPrimitive);
    CPPUNIT_TEST(testWalkSymbol);
    CPPUNIT_TEST(testWalkSymbolWithFullCache);
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() override {}
    inline void tearDown() override {}

private:
    static void testWalkArray();
    static void testWalkBlock();
    static void testWalkClass();
    static void testWalkDouble();
    static void testWalkEvaluationPrimitive();
    static void testWalkFrame();
    static void testWalkInteger();
    static void testWalkString();
    static void testWalkMethod();
    static void testWalkObject();
    static void testWalkPrimitive();
    static void testWalkSymbol();
    static void testWalkSymbolWithFullCache();
};
