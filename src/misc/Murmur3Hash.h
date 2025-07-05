#pragma once

#include <cstddef>
#include <cstdint>

// Computes a 32-bit Murmur3 hash of the input key.
uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);
