/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the content table lifecycle: entry creation, ci
    allocation, content setup, stat and destroy (nft32/ctable.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_ctable_and_entry_create() {
    directory_entry_t meta = { 0 };
    cinfo_t info = { 0 };
    ci_t ci;

    UT_ASSERT(create_entry("FILE    TXT", 0, 9, 1234, &meta) == 1);
    UT_ASSERT(memcmp(meta.file_name, "FILE    TXT", 11) == 0);
    UT_ASSERT(meta.dca == 9);
    UT_ASSERT(meta.file_size == 1234);
    UT_ASSERT(meta.attributes == FILE_ARCHIVE);
    UT_ASSERT(meta.name_hash == nft32_murmur3_x86_32(meta.file_name, sizeof(meta.file_name), 0));

    ctable_init();
    ci = alloc_ci();
    UT_ASSERT(OPEN_SUCCESS(ci));
    UT_ASSERT(setup_content(ci, 0, "FILE    TXT", &meta, 0x03) == 1);
    UT_ASSERT(get_content_data_ca(ci) == 9);
    UT_ASSERT(get_content_size(ci) == 1234);
    UT_ASSERT(get_content_mode(ci) == 0x03);
    UT_ASSERT(get_content_type(ci) == CONTENT_TYPE_FILE);
    UT_ASSERT(stat_content(ci, &info) == 1);
    UT_ASSERT(info.type == STAT_FILE);
    UT_ASSERT(memcmp(info.full_name, "FILE    TXT", 11) == 0);
    UT_ASSERT(destroy_content(ci) == 1);
    return 1;
}
