/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the filesystem memory manager (std/mm.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_memory_manager() {
    void* a = nft32_malloc_s(13);
    void* b = nft32_malloc_s(29);
    UT_ASSERT(a != NULL);
    UT_ASSERT(b != NULL);
    UT_ASSERT(a != b);
    memset(a, 0x11, 13);
    memset(b, 0x22, 29);
    nft32_free_s(a);
    nft32_free_s(b);
    UT_ASSERT(nft32_malloc_s(0) == NULL);
    return 1;
}
