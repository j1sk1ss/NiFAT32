/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Common wrapper for NIFAT32 unit tests. Provides the assertion macro,
    an in-memory fake disk, pristine formatted image handling and single
    filesystem run helpers (reset image + init + operation + unload).
    Test bodies live in separate test_*.c files, the runner is utest_main.c.

Dependencies:
    - ../nifat32.h - Public NIFAT32 API.
    - stdio.h - Standard I/O declarations.
    - string.h - String utilities.
    - stdlib.h - Standard library utilities.
*/

#ifndef NIFAT32_UTEST_
#define NIFAT32_UTEST_

#include "../nifat32.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Sector size of the pristine image (must match the Makefile image geometry) */
#define UT_SECTOR_SIZE 512

/* Test counters (defined in utest_main.c) */
extern int utest_assertions;
extern int utest_failures;

#define UT_ASSERT(expr) do {                                                       \
    utest_assertions++;                                                            \
    if (!(expr)) {                                                                 \
        utest_failures++;                                                          \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);            \
        return 0;                                                                  \
    }                                                                              \
} while (0)

/* In-memory fake disk (implemented in utest_main.c) */
int utest_disk_setup(int sector_size, int sectors);
unsigned char* utest_disk_sector(sector_addr_t sa);
int utest_disk_read_calls();
int utest_disk_write_calls();

/* Pristine formatted image for single filesystem runs */
int utest_image_load(const char* path);
int utest_image_ready();
int utest_image_sectors(); // image size in sectors

/* Single filesystem run helpers (reset image + NIFAT32_init / NIFAT32_unload) */
int utest_fs_init();
int utest_fs_shutdown();

/* Tool tests: pure helpers, no disk */
int utest_str_helpers();
int utest_hamming_roundtrip_and_repair();
int utest_checksum_vectors();
int utest_fatname_helpers();
int utest_threading_locks();
int utest_memory_manager();

/* Tool tests: disk structures on the fake disk */
int utest_disk_io();
int utest_cluster_io();
int utest_fatmap();
int utest_entry_add_search_remove();
int utest_ecache();
int utest_ctable_and_entry_create();

/* Single filesystem runs on the pristine image */
int utest_fs_open_write_read();
int utest_fs_delete();
int utest_fs_rename();
int utest_fs_copy();
int utest_fs_truncate();
int utest_fs_stat();
int utest_fs_nested_create();
int utest_fs_content_exists();

#endif
