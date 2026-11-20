/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the directory entry add/search/remove cycle on the
    fake disk (nft32/entry.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

#define TEST_SECTOR_SIZE 64
#define TEST_SECTORS     128

int utest_entry_add_search_remove() {
    fat_data_t fi = {
        .fat_size = 2,
        .fat_count = 1,
        .first_data_sector = 20,
        .total_sectors = TEST_SECTORS,
        .total_clusters = 16,
        .bytes_per_sector = TEST_SECTOR_SIZE,
        .sectors_per_cluster = 2,
        .cluster_size = TEST_SECTOR_SIZE * 2,
        .ext_root_cluster = 2,
        .sectors_padd = 1,
        .journals_count = 0
    };
    directory_entry_t meta = { 0 };
    directory_entry_t found = { 0 };

    UT_ASSERT(utest_disk_setup(TEST_SECTOR_SIZE, TEST_SECTORS) == 1);
    UT_ASSERT(write_fat(2, FAT_CLUSTER_END, &fi) == 1);
    UT_ASSERT(write_fat(3, FAT_CLUSTER_END, &fi) == 1);
    UT_ASSERT(create_entry("UNIT    BIN", 0, 3, 77, &meta) == 1);
    UT_ASSERT(entry_add(2, NO_ECACHE, &meta, &fi) == 1);
    UT_ASSERT(entry_search("UNIT    BIN", 2, NO_ECACHE, &found, &fi) == 1);
    UT_ASSERT(memcmp(found.file_name, "UNIT    BIN", 11) == 0);
    UT_ASSERT(found.dca == 3);
    UT_ASSERT(found.file_size == 77);
    UT_ASSERT(entry_remove(2, "UNIT    BIN", NO_ECACHE, &fi) == 1);
    UT_ASSERT(entry_search("UNIT    BIN", 2, NO_ECACHE, &found, &fi) == 0);
    UT_ASSERT(read_fat(3, &fi) == FAT_CLUSTER_FREE);
    return 1;
}
