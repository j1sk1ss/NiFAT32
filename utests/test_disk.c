/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the sector I/O primitives on the fake disk (nft32/disk.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

#define TEST_SECTOR_SIZE 64
#define TEST_SECTORS     128

int utest_disk_io() {
    unsigned char src[80];
    unsigned char dst[80] = { 0 };
    unsigned char copy[TEST_SECTOR_SIZE] = { 0 };

    UT_ASSERT(utest_disk_setup(TEST_SECTOR_SIZE, TEST_SECTORS) == 1);
    for (unsigned int i = 0; i < sizeof(src); i++) src[i] = (unsigned char)(i + 3);

    UT_ASSERT(DSK_get_sector_size() == TEST_SECTOR_SIZE);
    UT_ASSERT(DSK_writeoff_sectors(3, 20, src, sizeof(src), 2) == 1);
    UT_ASSERT(utest_disk_write_calls() == 2);
    UT_ASSERT(DSK_readoff_sectors(3, 20, dst, sizeof(dst), 2) == 1);
    UT_ASSERT(utest_disk_read_calls() == 2);
    UT_ASSERT(memcmp(src, dst, sizeof(src)) == 0);
    UT_ASSERT(DSK_copy_sectors(3, 8, 2, copy, sizeof(copy)) == 4);
    UT_ASSERT(memcmp(utest_disk_sector(3), utest_disk_sector(8), TEST_SECTOR_SIZE) == 0);
    UT_ASSERT(memcmp(utest_disk_sector(4), utest_disk_sector(9), TEST_SECTOR_SIZE) == 0);
    return 1;
}
