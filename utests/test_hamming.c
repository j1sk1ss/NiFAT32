/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the Hamming(15,11) pack/unpack roundtrip with single bit
    flip repair and in-place decoding (std/hamming.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_hamming_roundtrip_and_repair() {
    const byte_t src[] = { 0x00, 0x01, 0x7F, 0x80, 0xAA, 0x55, 0xFE, 0xFF };
    encoded_t encoded[sizeof(src)] = { 0 };
    byte_t decoded[sizeof(src)] = { 0 };
    encoded_t inplace[sizeof(src)] = { 0 };

    nft32_pack_memory(src, encoded, sizeof(src));
#ifndef NO_HAMMING
    encoded[3] ^= 1U;
#endif
    nft32_unpack_memory(encoded, decoded, sizeof(src));
    UT_ASSERT(memcmp(decoded, src, sizeof(src)) == 0);

    nft32_pack_memory(src, inplace, sizeof(src));
    nft32_unpack_memory(inplace, (byte_t*)inplace, sizeof(src));
    UT_ASSERT(memcmp((byte_t*)inplace, src, sizeof(src)) == 0);
    return 1;
}
