#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../misc/defs.h"

class LargeObjectTest : public CPPUNIT_NS::TestCase {
    CPPUNIT_TEST_SUITE(LargeObjectTest);  // NOLINT(misc-const-correctness)
    // MARK_SWEEP and COPYING have fixed-size heaps, which only grow during a
    // collection, so they can't allocate large objects when nearly full.
#if GC_TYPE != MARK_SWEEP && GC_TYPE != COPYING
    CPPUNIT_TEST(testLargeObjectsSurviveGC);
    CPPUNIT_TEST(testLargeObjectsRequestGC);
#endif
#if GC_TYPE == GENERATIONAL
    CPPUNIT_TEST(testTooLargeForNurseryGoesToMatureSpace);
    CPPUNIT_TEST(testFullNurseryAllocatesInMatureSpace);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    inline void setUp() override {}
    inline void tearDown() override {}

private:
    static void testLargeObjectsSurviveGC();
    static void testLargeObjectsRequestGC();
#if GC_TYPE == GENERATIONAL
    static void testTooLargeForNurseryGoesToMatureSpace();
    static void testFullNurseryAllocatesInMatureSpace();
#endif
};
