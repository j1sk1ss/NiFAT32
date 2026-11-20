/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the FAT free cluster map (nft32/fatmap.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_fatmap() {
    fat_data_t fi = { .total_clusters = 64 };

    UT_ASSERT(fatmap_init(&fi) == 1);
    UT_ASSERT(fatmap_unset(0) == 1);
    UT_ASSERT(fatmap_unset(1) == 1);
    UT_ASSERT(fatmap_unset(2) == 1);
    UT_ASSERT(fatmap_find_free(0, 1, &fi) == 3);
    UT_ASSERT(fatmap_set(1) == 1);
    UT_ASSERT(fatmap_find_free(0, 2, &fi) == 3);
    UT_ASSERT(fatmap_unload() == 1);
    return 1;
}
