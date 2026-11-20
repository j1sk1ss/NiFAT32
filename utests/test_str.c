/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the string and memory helpers (std/str.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_str_helpers() {
    unsigned char src[] = { 1, 2, 3, 4, 5, 6, 7 };
    unsigned char dst[sizeof(src)] = { 0 };
    char text[16] = { 0 };

    UT_ASSERT(nft32_str_memcpy(dst, src, sizeof(src)) == dst);
    UT_ASSERT(memcmp(dst, src, sizeof(src)) == 0);
    UT_ASSERT(nft32_str_memset(dst, 0xAB, sizeof(dst)) == dst);
    for (unsigned int i = 0; i < sizeof(dst); i++) UT_ASSERT(dst[i] == 0xAB);
    UT_ASSERT(nft32_str_memcmp("abc", "abc", 3) == 0);
    UT_ASSERT(nft32_str_strncmp("abc", "abd", 3) < 0);
    UT_ASSERT(nft32_str_strlen("hello") == 5);
    UT_ASSERT(nft32_str_strcpy(text, "nifat") == text);
    UT_ASSERT(strcmp(text, "nifat") == 0);
    UT_ASSERT(nft32_str_uppercase(text) == 1);
    UT_ASSERT(strcmp(text, "NIFAT") == 0);
    return 1;
}
