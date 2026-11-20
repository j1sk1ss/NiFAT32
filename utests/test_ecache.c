/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the red-black entry cache (nft32/ecache.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_ecache() {
    ecache_t* root = NULL;

    root = ecache_insert(root, 30, 1, 300);
    root = ecache_insert(root, 10, 0, 100);
    root = ecache_insert(root, 20, 1, 200);
    root = ecache_insert(root, 40, 0, 400);

    UT_ASSERT(root != NULL);
    UT_ASSERT(ecache_find(root, 10)->ca == 100);
    UT_ASSERT(ecache_find(root, 20)->ca == 200);
    UT_ASSERT(IS_ECACHE_DIR(ecache_find(root, 20)));
    UT_ASSERT(IS_ECACHE_FILE(ecache_find(root, 40)));
    root = ecache_delete(root, 10);
    UT_ASSERT(ecache_find(root, 10) == NULL);
    UT_ASSERT(ecache_find(root, 30)->ca == 300);
    UT_ASSERT(ecache_free(root) == 1);
    return 1;
}
