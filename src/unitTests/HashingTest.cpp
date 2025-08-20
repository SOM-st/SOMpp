#include "HashingTest.h"

#include <cppunit/TestAssert.h>
#include <cstdint>
#include <cstring>

#include "../misc/Murmur3Hash.h"

void HashingTest::testMurmur3HashWithSeeds() {
    // Hex keys
    const char* input1 = "";

    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    const uint8_t* in1 = reinterpret_cast<const uint8_t*>(input1);

    CPPUNIT_ASSERT_EQUAL(0x00000000U,
                         murmur3_32(in1, strlen(input1), 0x00000000));
    CPPUNIT_ASSERT_EQUAL(0x514e28b7U,
                         murmur3_32(in1, strlen(input1), 0x00000001));
    CPPUNIT_ASSERT_EQUAL(0x81f16f39U,
                         murmur3_32(in1, strlen(input1), 0xffffffff));

    // Strings
    const char* str1 = "test";

    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    const uint8_t* s1 = reinterpret_cast<const uint8_t*>(str1);

    CPPUNIT_ASSERT_EQUAL(0xba6bd213U, murmur3_32(s1, strlen(str1), 0x00000000));
    CPPUNIT_ASSERT_EQUAL(0x704b81dcU, murmur3_32(s1, strlen(str1), 0x9747b28c));

    const char* str2 = "Hello, world!";

    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    const uint8_t* s2 = reinterpret_cast<const uint8_t*>(str2);

    CPPUNIT_ASSERT_EQUAL(0xc0363e43U, murmur3_32(s2, strlen(str2), 0x00000000));
    CPPUNIT_ASSERT_EQUAL(0x24884cbaU, murmur3_32(s2, strlen(str2), 0x9747b28c));

    const char* str3 = "The quick brown fox jumps over the lazy dog";

    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    const uint8_t* s3 = reinterpret_cast<const uint8_t*>(str3);

    CPPUNIT_ASSERT_EQUAL(0x2e4ff723U, murmur3_32(s3, strlen(str3), 0x00000000));
    CPPUNIT_ASSERT_EQUAL(0x2fa826cdU, murmur3_32(s3, strlen(str3), 0x9747b28c));
}
