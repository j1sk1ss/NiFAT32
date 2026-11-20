/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test runner. Owns the global test counters, the in-memory
    fake disk, the pristine formatted image and the main test loop.

Dependencies:
    - utest.h - Common unit test wrapper.
    - stdarg.h - Variable argument lists (log mock).
*/

#include "utest.h"
#include <stdarg.h>

int utest_assertions = 0;
int utest_failures = 0;

/* In-memory fake disk */
static unsigned char* _utest_disk = NULL;
static int _utest_sector_size = 0;
static unsigned int _utest_sectors = 0;
static int _utest_read_calls = 0;
static int _utest_write_calls = 0;

static int _utest_disk_read(sector_addr_t sa, sector_offset_t off, unsigned char* buffer, int size) {
    if (sa >= _utest_sectors || off < 0 || off + size > _utest_sector_size) return 0;
    memcpy(buffer, _utest_disk + (size_t)(sa * _utest_sector_size) + off, size);
    _utest_read_calls++;
    return 1;
}

static int _utest_disk_write(sector_addr_t sa, sector_offset_t off, const unsigned char* data, int size) {
    if (sa >= _utest_sectors || off < 0 || off + size > _utest_sector_size) return 0;
    memcpy(_utest_disk + (size_t)(sa * _utest_sector_size) + off, data, size);
    _utest_write_calls++;
    return 1;
}

int utest_disk_setup(int sector_size, int sectors) {
    if (_utest_disk) free(_utest_disk);
    _utest_disk = NULL;
    _utest_sector_size = sector_size;
    _utest_sectors = (unsigned int)sectors;
    _utest_read_calls = 0;
    _utest_write_calls = 0;

    _utest_disk = calloc((size_t)sectors * (size_t)sector_size, 1);
    if (!_utest_disk) return 0;

    return DSK_setup(_utest_disk_read, _utest_disk_write, sector_size);
}

unsigned char* utest_disk_sector(sector_addr_t sa) {
    if (sa >= _utest_sectors) return NULL;
    return _utest_disk + (size_t)(sa * _utest_sector_size);
}

int utest_disk_read_calls() {
    return _utest_read_calls;
}

int utest_disk_write_calls() {
    return _utest_write_calls;
}

/* Pristine formatted image */
static unsigned char* _utest_image = NULL;
static unsigned int _utest_image_sectors = 0;

int utest_image_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }

    long size = ftell(f);
    if (size <= 0 || size % UT_SECTOR_SIZE) {
        fclose(f);
        return 0;
    }

    rewind(f);
    _utest_image = malloc((size_t)size);
    if (!_utest_image || fread(_utest_image, 1, (size_t)size, f) != (size_t)size) {
        free(_utest_image);
        _utest_image = NULL;
        fclose(f);
        return 0;
    }

    fclose(f);
    _utest_image_sectors = (unsigned int)(size / UT_SECTOR_SIZE);
    return 1;
}

int utest_image_ready() {
    return _utest_image != NULL;
}

int utest_image_sectors() {
    return (int)_utest_image_sectors;
}

/* Single filesystem run */
/* Geometry of the pristine image (must match the Makefile `unit` target) */
#define UT_FS_BS_COUNT 5

static int _utest_log_fprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vfprintf(stdout, fmt, args);
    va_end(args);
    return result;
}

static int _utest_log_vfprintf(const char* fmt, va_list args) {
    return vfprintf(stdout, fmt, args);
}

int utest_fs_init() {
    if (!utest_image_ready()) return 0;

    if (!utest_disk_setup(UT_SECTOR_SIZE, _utest_image_sectors)) return 0;
    memcpy(_utest_disk, _utest_image, (size_t)_utest_image_sectors * UT_SECTOR_SIZE);

    nifat32_params_t params = {
        .bs_num    = 0,
        .ts        = (unsigned int)_utest_image_sectors,
        .jc        = 0,
        .bs_count  = UT_FS_BS_COUNT,
        .fat_cache = CACHE,
        .disk_io   = {
            .read_sector  = _utest_disk_read,
            .write_sector = _utest_disk_write,
            .sector_size  = UT_SECTOR_SIZE
        },
        .logg_io   = {
            .fd_fprintf  = _utest_log_fprintf,
            .fd_vfprintf = _utest_log_vfprintf
        },
        .mm_manager = { DEFAULT_MM_MANAGER },
        .ef         = -1
    };

    return NIFAT32_init(&params);
}

int utest_fs_shutdown() {
    return NIFAT32_unload();
}

static void _utest_run(const char* name, int (*test)()) {
    fprintf(stderr, "utest %-32s ", name);
    if (test()) fprintf(stderr, "OK\n");
    else        fprintf(stderr, "FAILED\n");
}

int main(int argc, char* argv[]) {
    nft32_setup_mm_manager(DEFAULT_MM_MANAGER);
    nft32_mm_init();

    const char* image_path = argc > 1 ? argv[1] : "nifat32.img";
    int image_ready = utest_image_load(image_path);
    if (image_ready) {
        fprintf(stderr, "Image loaded: %s (%i sectors)\n", image_path, utest_image_sectors());
    }
    else {
        fprintf(stderr, "Image not loaded (%s), single FS runs will be skipped\n", image_path);
    }

    // Tool tests: pure helpers, no disk
    _utest_run("str_helpers", utest_str_helpers);
    _utest_run("hamming_roundtrip_and_repair", utest_hamming_roundtrip_and_repair);
    _utest_run("checksum_vectors", utest_checksum_vectors);
    _utest_run("fatname_helpers", utest_fatname_helpers);
    _utest_run("threading_locks", utest_threading_locks);
    _utest_run("memory_manager", utest_memory_manager);

    // Tool tests: disk structures on the fake disk
    _utest_run("disk_io", utest_disk_io);
    _utest_run("cluster_io", utest_cluster_io);
    _utest_run("fatmap", utest_fatmap);
    _utest_run("entry_add_search_remove", utest_entry_add_search_remove);
    _utest_run("ecache", utest_ecache);
    _utest_run("ctable_and_entry_create", utest_ctable_and_entry_create);

    // Single filesystem runs on the pristine image
    if (image_ready) {
        _utest_run("fs_open_write_read", utest_fs_open_write_read);
        _utest_run("fs_delete", utest_fs_delete);
        _utest_run("fs_rename", utest_fs_rename);
        _utest_run("fs_copy", utest_fs_copy);
        _utest_run("fs_truncate", utest_fs_truncate);
        _utest_run("fs_stat", utest_fs_stat);
        _utest_run("fs_nested_create", utest_fs_nested_create);
        _utest_run("fs_content_exists", utest_fs_content_exists);
    }

    fprintf(stderr, "utests: %i assertions, %i failed\n", utest_assertions, utest_failures);
    return utest_failures ? 1 : 0;
}
