#include "HashingTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ios>
#include <sstream>

#include "../misc/Murmur3Hash.h"

void HashingTest::testMurmur3HashWithSeeds() {
    // Hex keys
    const char* input1 = "";
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(input1),
                            strlen(input1), 0x00000000, 0x00000000);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(input1),
                            strlen(input1), 0x00000001, 0x514E28B7);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(input1),
                            strlen(input1), 0xffffffff, 0x81F16F39);

    // Strings
    const char* str1 = "test";
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str1),
                            strlen(str1), 0x00000000, 0xba6bd213);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str1),
                            strlen(str1), 0x9747b28c, 0x704b81dc);

    const char* str2 = "Hello, world!";
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str2),
                            strlen(str2), 0x00000000, 0xc0363e43);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str2),
                            strlen(str2), 0x9747b28c, 0x24884CBA);

    const char* str3 = "The quick brown fox jumps over the lazy dog";
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str3),
                            strlen(str3), 0x00000000, 0x2e4ff723);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    testMurmur3HashWithSeed(reinterpret_cast<const uint8_t*>(str3),
                            strlen(str3), 0x9747b28c, 0x2FA826CD);
}

void HashingTest::testMurmur3HashWithSeed(const void* key, size_t len,
                                          uint32_t seed,
                                          uint32_t expectedHash) {
    const uint32_t actualHash =
        Murmur3Hash::murmur3_32(static_cast<const uint8_t*>(key), len, seed);

    std::ostringstream msg;
    msg << "Expected hash 0x" << std::hex << expectedHash << " but got 0x"
        << actualHash;

    CPPUNIT_ASSERT_MESSAGE(msg.str(), actualHash == expectedHash);
}
