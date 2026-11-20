/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Single filesystem runs. Each test resets the pristine formatted image,
    inits NIFAT32, performs exactly one operation and unloads the
    filesystem. Runs on the in-memory fake disk (no file I/O).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_fs_open_write_read() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit1/file.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));

    const char data[] = "nifat32 unit fs single run data!";
    UT_ASSERT(NIFAT32_write_buffer2content(ci, 0, (const_buffer_t)data, (int)sizeof(data)) == (int)sizeof(data));

    char buffer[sizeof(data)] = { 0 };
    UT_ASSERT(NIFAT32_read_content2buffer(ci, 0, (buffer_t)buffer, sizeof(buffer)) == (int)sizeof(data));
    UT_ASSERT(memcmp(buffer, data, sizeof(data)) == 0);

    UT_ASSERT(NIFAT32_close_content(ci) == 1);
    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_delete() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit2/file.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));

    const char data[] = "delete me";
    UT_ASSERT(NIFAT32_write_buffer2content(ci, 0, (const_buffer_t)data, (int)sizeof(data)) == (int)sizeof(data));
    UT_ASSERT(NIFAT32_close_content(ci) == 1);

    UT_ASSERT(NIFAT32_content_exists("unit2/file.txt") == 1);
    ci = NIFAT32_open_content(NO_RCI, "unit2/file.txt", DF_MODE);
    UT_ASSERT(OPEN_SUCCESS(ci));
    UT_ASSERT(NIFAT32_delete_content(ci) == 1);
    UT_ASSERT(NIFAT32_content_exists("unit2/file.txt") == 0);
    ci = NIFAT32_open_content(NO_RCI, "unit2/file.txt", R_MODE);
    UT_ASSERT(ci < 0);

    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_rename() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit3/old.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));

    const char data[] = "rename payload";
    UT_ASSERT(NIFAT32_write_buffer2content(ci, 0, (const_buffer_t)data, (int)sizeof(data)) == (int)sizeof(data));

    cinfo_t st = { 0 };
    UT_ASSERT(NIFAT32_stat_content(ci, &st) == 1);

    cinfo_t info = { .type = STAT_FILE, .size = st.size };
    nft32_str_memcpy(info.name, "new", 3);
    nft32_str_memcpy(info.extention, "dat", 3);
    nft32_name_to_fatname("new.dat", info.full_name);
    UT_ASSERT(NIFAT32_change_meta(ci, &info) == 1);
    UT_ASSERT(NIFAT32_close_content(ci) == 1);

    UT_ASSERT(NIFAT32_content_exists("unit3/old.txt") == 0);
    UT_ASSERT(NIFAT32_content_exists("unit3/new.dat") == 1);
    ci = NIFAT32_open_content(NO_RCI, "unit3/new.dat", R_MODE);
    UT_ASSERT(OPEN_SUCCESS(ci));

    char buffer[sizeof(data)] = { 0 };
    UT_ASSERT(NIFAT32_read_content2buffer(ci, 0, (buffer_t)buffer, sizeof(buffer)) == (int)sizeof(data));
    UT_ASSERT(memcmp(buffer, data, sizeof(data)) == 0);

    UT_ASSERT(NIFAT32_close_content(ci) == 1);
    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_copy() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t src = NIFAT32_open_content(NO_RCI, "unit4/src.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    ci_t dst = NIFAT32_open_content(NO_RCI, "unit4/dst.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(src) && OPEN_SUCCESS(dst));

    const char data[] = "deep copy payload data";
    UT_ASSERT(NIFAT32_write_buffer2content(src, 0, (const_buffer_t)data, (int)sizeof(data)) == (int)sizeof(data));
    UT_ASSERT(NIFAT32_copy_content(src, dst, DEEP_COPY) == 1);

    /* Note: DEEP_COPY syncs the data, the dst entry size is left untouched */
    char buffer[sizeof(data)] = { 0 };
    UT_ASSERT(NIFAT32_read_content2buffer(dst, 0, (buffer_t)buffer, sizeof(buffer)) == (int)sizeof(data));
    UT_ASSERT(memcmp(buffer, data, sizeof(data)) == 0);

    UT_ASSERT(NIFAT32_close_content(src) == 1);
    UT_ASSERT(NIFAT32_close_content(dst) == 1);
    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_truncate() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit5/trunc.bin", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));

    unsigned char data[8192];
    for (unsigned int i = 0; i < sizeof(data); i++) data[i] = (unsigned char)(i & 0xFF);
    UT_ASSERT(NIFAT32_write_buffer2content(ci, 0, (const_buffer_t)data, sizeof(data)) == sizeof(data));

    UT_ASSERT(NIFAT32_truncate_content(ci, 0, 2048) == 1);
    UT_ASSERT(NIFAT32_close_content(ci) == 1);

    /* The entry is updated on disk, so re-open to check the new size */
    ci = NIFAT32_open_content(NO_RCI, "unit5/trunc.bin", R_MODE);
    UT_ASSERT(OPEN_SUCCESS(ci));

    cinfo_t info = { 0 };
    UT_ASSERT(NIFAT32_stat_content(ci, &info) == 1);
    UT_ASSERT(info.size == 2048);

    unsigned char buffer[2048] = { 0 };
    UT_ASSERT(NIFAT32_read_content2buffer(ci, 0, (buffer_t)buffer, sizeof(buffer)) == 2048);
    UT_ASSERT(memcmp(buffer, data, 2048) == 0);

    UT_ASSERT(NIFAT32_close_content(ci) == 1);
    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_stat() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t dir = NIFAT32_open_content(NO_RCI, "unit6", MODE(CR_MODE, DIR_TARGET));
    ci_t file = NIFAT32_open_content(NO_RCI, "unit6/file.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(dir) && OPEN_SUCCESS(file));

    cinfo_t info = { 0 };
    UT_ASSERT(NIFAT32_stat_content(file, &info) == 1);
    UT_ASSERT(info.type == STAT_FILE);
    UT_ASSERT(strstr(info.full_name, "FILE") != NULL);
    UT_ASSERT(NIFAT32_close_content(file) == 1);

    UT_ASSERT(NIFAT32_stat_content(dir, &info) == 1);
    UT_ASSERT(info.type == STAT_DIR);
    UT_ASSERT(NIFAT32_close_content(dir) == 1);

    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_nested_create() {
    UT_ASSERT(utest_fs_init() == 1);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit7/a/b/c/deep.txt", MODE(CR_MODE | W_MODE | R_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));
    UT_ASSERT(NIFAT32_close_content(ci) == 1);

    UT_ASSERT(NIFAT32_content_exists("unit7/a/b/c") == 1);
    UT_ASSERT(NIFAT32_content_exists("unit7/a/b/c/deep.txt") == 1);

    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}

int utest_fs_content_exists() {
    UT_ASSERT(utest_fs_init() == 1);

    UT_ASSERT(NIFAT32_content_exists("unit8/ghost.txt") == 0);

    ci_t ci = NIFAT32_open_content(NO_RCI, "unit8/real.txt", MODE(CR_MODE, FILE_TARGET));
    UT_ASSERT(OPEN_SUCCESS(ci));
    UT_ASSERT(NIFAT32_close_content(ci) == 1);

    UT_ASSERT(NIFAT32_content_exists("unit8/real.txt") == 1);

    UT_ASSERT(utest_fs_shutdown() == 1);
    return 1;
}
