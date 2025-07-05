#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "TestWithParsing.h"

using namespace std;

class HashingTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(HashingTest);  // NOLINT(misc-const-correctness)
    CPPUNIT_TEST(testMurmur3HashWithSeeds);
    CPPUNIT_TEST_SUITE_END();

private:
    static void testMurmur3HashWithSeeds();
};
