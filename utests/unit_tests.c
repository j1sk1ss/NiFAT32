#include "../nifat32.h"

#include <stdio.h>
#include <string.h>

#define TEST_SECTOR_SIZE 64
#define TEST_SECTORS     128

static int _tests_run = 0;
static int _tests_failed = 0;

static unsigned char _disk[TEST_SECTORS][TEST_SECTOR_SIZE];
static int _read_calls = 0;
static int _write_calls = 0;

#define UT_ASSERT(expr) do {                                                        \
    _tests_run++;                                                                   \
    if (!(expr)) {                                                                  \
        _tests_failed++;                                                            \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);             \
        return 0;                                                                   \
    }                                                                               \
} while (0)

#define UT_RUN(test) do {                                                           \
    fprintf(stderr, "utest %-32s", #test);                                          \
    if (test()) fprintf(stderr, "OK\n");                                           \
    else fprintf(stderr, "FAILED\n");                                              \
} while (0)

static int _fake_read(sector_addr_t sa, sector_offset_t off, unsigned char* buffer, int size) {
    if (sa >= TEST_SECTORS || off < 0 || off + size > TEST_SECTOR_SIZE) return 0;
    memcpy(buffer, _disk[sa] + off, size);
    _read_calls++;
    return 1;
}

static int _fake_write(sector_addr_t sa, sector_offset_t off, const unsigned char* data, int size) {
    if (sa >= TEST_SECTORS || off < 0 || off + size > TEST_SECTOR_SIZE) return 0;
    memcpy(_disk[sa] + off, data, size);
    _write_calls++;
    return 1;
}

static void _reset_disk(void) {
    memset(_disk, 0, sizeof(_disk));
    _read_calls = 0;
    _write_calls = 0;
    DSK_setup(_fake_read, _fake_write, TEST_SECTOR_SIZE);
}

static int test_str_helpers(void) {
    unsigned char src[] = { 1, 2, 3, 4, 5, 6, 7 };
    unsigned char dst[sizeof(src)] = { 0 };
    char text[16] = { 0 };

    UT_ASSERT(nft32_str_memcpy(dst, src, sizeof(src)) == dst);
    UT_ASSERT(memcmp(dst, src, sizeof(src)) == 0);
    UT_ASSERT(nft32_str_memset(dst, 0xAB, sizeof(dst)) == dst);
    for (unsigned int i = 0; i < sizeof(dst); i++) UT_ASSERT(dst[i] == 0xAB);
    UT_ASSERT(nft32_str_memcmp("abc", "abc", 3) == 0);
    UT_ASSERT(nft32_str_strncmp("abc", "abd", 3) < 0);
    UT_ASSERT(nft32_str_strlen("hello") == 5);
    UT_ASSERT(nft32_str_strcpy(text, "nifat") == text);
    UT_ASSERT(strcmp(text, "nifat") == 0);
    UT_ASSERT(nft32_str_uppercase(text) == 1);
    UT_ASSERT(strcmp(text, "NIFAT") == 0);
    return 1;
}

static int test_hamming_roundtrip_and_repair(void) {
    const byte_t src[] = { 0x00, 0x01, 0x7F, 0x80, 0xAA, 0x55, 0xFE, 0xFF };
    encoded_t encoded[sizeof(src)] = { 0 };
    byte_t decoded[sizeof(src)] = { 0 };
    encoded_t inplace[sizeof(src)] = { 0 };

    nft32_pack_memory(src, encoded, sizeof(src));
#ifndef NO_HAMMING
    encoded[3] ^= 1U;
#endif
    nft32_unpack_memory(encoded, decoded, sizeof(src));
    UT_ASSERT(memcmp(decoded, src, sizeof(src)) == 0);

    nft32_pack_memory(src, inplace, sizeof(src));
    nft32_unpack_memory(inplace, (byte_t*)inplace, sizeof(src));
    UT_ASSERT(memcmp((byte_t*)inplace, src, sizeof(src)) == 0);
    return 1;
}

static int test_checksum_vectors(void) {
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"", 0, 0) == 0U);
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"hello", 5, 0) == 0x248BFA47U);
    UT_ASSERT(nft32_murmur3_x86_32((const unsigned char*)"NIFAT32", 7, 123) == 0xE5D8AC74U);
    return 1;
}

static int test_fatname_helpers(void) {
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

static int test_threading_locks(void) {
    lock_t lock = NULL_LOCK;
    UT_ASSERT(THR_require_read(&lock) == 1);
    UT_ASSERT(THR_require_read(&lock) == 1);
    UT_ASSERT(THR_require_write(&lock, 7) == 0);
    UT_ASSERT(THR_release_read(&lock) == 1);
    UT_ASSERT(THR_release_read(&lock) == 1);
    UT_ASSERT(THR_require_write(&lock, 7) == 1);
    UT_ASSERT(THR_release_write(&lock, 8) == 0);
    UT_ASSERT(THR_release_write(&lock, 7) == 1);
    return 1;
}

static int test_memory_manager(void) {
    void* a = nft32_malloc_s(13);
    void* b = nft32_malloc_s(29);
    UT_ASSERT(a != NULL);
    UT_ASSERT(b != NULL);
    UT_ASSERT(a != b);
    memset(a, 0x11, 13);
    memset(b, 0x22, 29);
    nft32_free_s(a);
    nft32_free_s(b);
    UT_ASSERT(nft32_malloc_s(0) == NULL);
    return 1;
}

static int test_disk_io(void) {
    unsigned char src[80];
    unsigned char dst[80] = { 0 };
    unsigned char copy[TEST_SECTOR_SIZE] = { 0 };

    _reset_disk();
    for (unsigned int i = 0; i < sizeof(src); i++) src[i] = (unsigned char)(i + 3);

    UT_ASSERT(DSK_get_sector_size() == TEST_SECTOR_SIZE);
    UT_ASSERT(DSK_writeoff_sectors(3, 20, src, sizeof(src), 2) == 1);
    UT_ASSERT(_write_calls == 2);
    UT_ASSERT(DSK_readoff_sectors(3, 20, dst, sizeof(dst), 2) == 1);
    UT_ASSERT(_read_calls == 2);
    UT_ASSERT(memcmp(src, dst, sizeof(src)) == 0);
    UT_ASSERT(DSK_copy_sectors(3, 8, 2, copy, sizeof(copy)) == 4);
    UT_ASSERT(memcmp(_disk[3], _disk[8], TEST_SECTOR_SIZE) == 0);
    UT_ASSERT(memcmp(_disk[4], _disk[9], TEST_SECTOR_SIZE) == 0);
    return 1;
}

static int test_cluster_io(void) {
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

    _reset_disk();
    for (unsigned int i = 0; i < sizeof(src); i++) src[i] = (unsigned char)(0x80 + i);

    UT_ASSERT(get_cluster_count(256, &fi) == 2);
    UT_ASSERT(writeoff_cluster(4, 10, src, sizeof(src), &fi) == 1);
    UT_ASSERT(readoff_cluster(4, 10, dst, sizeof(dst), &fi) == 1);
    UT_ASSERT(memcmp(src, dst, sizeof(src)) == 0);
    UT_ASSERT(copy_cluster(4, 5, copy, sizeof(copy), &fi) == 4);
    UT_ASSERT(memcmp(_disk[24], _disk[26], TEST_SECTOR_SIZE) == 0);
    UT_ASSERT(memcmp(_disk[25], _disk[27], TEST_SECTOR_SIZE) == 0);
    return 1;
}

static int test_fat_read_entire_table_bulk(void) {
    fat_data_t fi = {
        .sectors_padd = 2,
        .total_sectors = TEST_SECTORS,
        .bytes_per_sector = TEST_SECTOR_SIZE
    };
    cluster_val_t values[8] = {
        0x10000000U, 0x00000002U, FAT_CLUSTER_END, FAT_CLUSTER_FREE,
        0x00000004U, FAT_CLUSTER_RESERVED, FAT_CLUSTER_BAD, 0x0ABCDEF0U
    };
    encoded_t encoded[sizeof(values)] = { 0 };
    encoded_t dst_storage[sizeof(values)] = { 0 };
    cluster_val_t* dst = (cluster_val_t*)dst_storage;
    sector_addr_t fat_sector = fi.sectors_padd + GET_FATSECTOR(1, fi.total_sectors);

    _reset_disk();
    nft32_pack_memory((const byte_t*)values, encoded, sizeof(values));
    memcpy(_disk[fat_sector], encoded, sizeof(encoded));

    UT_ASSERT(fat_read_entire_table(1, dst, 8, &fi) == 1);
    UT_ASSERT(_read_calls == 1);
    for (unsigned int i = 0; i < 8; i++) UT_ASSERT(dst[i] == (values[i] & 0x0FFFFFFFU));
    return 1;
}

static int test_fatmap(void) {
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

static int test_entry_add_search_remove(void) {
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

    _reset_disk();
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

static int test_ecache(void) {
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

static int test_ctable_and_entry_create(void) {
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

int main(void) {
    nft32_setup_mm_manager(DEFAULT_MM_MANAGER);
    nft32_mm_init();
    _reset_disk();

    UT_RUN(test_str_helpers);
    UT_RUN(test_hamming_roundtrip_and_repair);
    UT_RUN(test_checksum_vectors);
    UT_RUN(test_fatname_helpers);
    UT_RUN(test_threading_locks);
    UT_RUN(test_memory_manager);
    UT_RUN(test_disk_io);
    UT_RUN(test_cluster_io);
    UT_RUN(test_fat_read_entire_table_bulk);
    UT_RUN(test_fatmap);
    UT_RUN(test_entry_add_search_remove);
    UT_RUN(test_ecache);
    UT_RUN(test_ctable_and_entry_create);

    fprintf(stderr, "utests: %d assertions, %d failed\n", _tests_run, _tests_failed);
    return _tests_failed ? 1 : 0;
}
