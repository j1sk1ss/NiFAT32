/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the FAT 8.3 name and path conversion helpers
    (std/fatname.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_fatname_helpers() {
    char fatname[12] = { 0 };
    char name[32] = { 0 };
    char path[64] = { 0 };
    char fname[32] = { 0 };
    char base[9] = { 0 };
    char ext[4] = { 0 };

    nft32_name_to_fatname("readme.txt", fatname);
    UT_ASSERT(strcmp(fatname, "README  TXT") == 0);
    nft32_fatname_to_name(fatname, name);
    UT_ASSERT(strcmp(name, "README.TXT") == 0);
    nft32_path_to_fatnames("root/dir/file.bin", path);
    UT_ASSERT(strcmp(path, "ROOT/DIR/FILE    BIN") == 0);
    nft32_extract_name("root/dir/file.bin", fname);
    UT_ASSERT(strcmp(fname, "file.bin") == 0);
    nft32_unpack_83_name("FILE    BIN", base, ext);
    UT_ASSERT(strcmp(base, "FILE    ") == 0);
    UT_ASSERT(strcmp(ext, "BIN") == 0);
    return 1;
}
