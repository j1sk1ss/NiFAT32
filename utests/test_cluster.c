/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the cluster I/O utilities on the fake disk
    (nft32/cluster.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

#define TEST_SECTOR_SIZE 64
#define TEST_SECTORS     128

int utest_cluster_io() {
    fat_data_t fi = {
        .first_data_sector = 20,
        .total_clusters = 32,
        .bytes_per_sector = TEST_SECTOR_SIZE,
        .sectors_per_cluster = 2,
        .cluster_size = TEST_SECTOR_SIZE * 2,
        .ext_root_cluster = 2
    };
    unsigned char src[90];
    unsigned char dst[90] = { 0 };
    unsigned char copy[TEST_SECTOR_SIZE] = { 0 };

    UT_ASSERT(utest_disk_setup(TEST_SECTOR_SIZE, TEST_SECTORS) == 1);
    for (unsigned int i = 0; i < sizeof(src); i++) src[i] = (unsigned char)(0x80 + i);

    UT_ASSERT(get_cluster_count(256, &fi) == 2);
    UT_ASSERT(writeoff_cluster(4, 10, src, sizeof(src), &fi) == 1);
    UT_ASSERT(readoff_cluster(4, 10, dst, sizeof(dst), &fi) == 1);
    UT_ASSERT(memcmp(src, dst, sizeof(src)) == 0);
    UT_ASSERT(copy_cluster(4, 5, copy, sizeof(copy), &fi) == 4);
    UT_ASSERT(memcmp(utest_disk_sector(24), utest_disk_sector(26), TEST_SECTOR_SIZE) == 0);
    UT_ASSERT(memcmp(utest_disk_sector(25), utest_disk_sector(27), TEST_SECTOR_SIZE) == 0);
    return 1;
}
