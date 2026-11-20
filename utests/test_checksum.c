/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the Murmur3-32 checksum against known vectors
    (std/checksum.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_checksum_vectors() {
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"", 0, 0) == 0U);
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"hello", 5, 0) == 0x248BFA47U);
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"NIFAT32", 7, 123) == 0xE5D8AC74U);
    return 1;
}
