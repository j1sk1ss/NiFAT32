#include <nft32/fat.h>

static cluster_val_t* _fat = NULL;
static lock_t _fat_lock = NULL_LOCK;

// TODO:
// int fat_read_entire_table(int index, cluster_val_t* dst, unsigned int size, fat_data_t* fi) {
//     cluster_offset_t fat_size = size * sizeof(cluster_val_t) * sizeof(encoded_t);
//     sector_addr_t fat_sector = fi->sectors_padd + GET_FATSECTOR(index, fi->total_sectors);

//     if (
//         !DSK_readoff_sectors(
//             fat_sector, 0, (unsigned char*)dst, fat_size, (fat_size + fi->bytes_per_sector - 1) / fi->bytes_per_sector
//         )
//     ) {
//         print_error("Could not read entire NiFAT32 table.");
//         errors_register_error(READ_FAT_ERROR, fi);
//         return 0;
//     }

//     nft32_unpack_memory((const encoded_t*)dst, (byte_t*)dst, size * sizeof(cluster_val_t));
//     for (cluster_addr_t ca = 0; ca < size; ca++) dst[ca] &= 0x0FFFFFFF;
//     return 1;
// }

int fat_cache_init(fat_data_t* fi) {
#ifndef NO_FAT_CACHE
    if (_fat) return 1;
    if (!THR_require_write(&_fat_lock, get_thread_num())) return 0;

    _fat = (cluster_val_t*)nft32_malloc_s(fi->total_clusters * sizeof(cluster_val_t));
    if (!_fat) {
        THR_release_write(&_fat_lock, get_thread_num());
        print_error("nft32_malloc_s() error!");
        errors_register_error(MALLOC_ERROR, fi);
        return 0;
    }

    for (cluster_addr_t ca = 0; ca < fi->total_clusters; ca++) _fat[ca] = FAT_CLUSTER_BAD;
    THR_release_write(&_fat_lock, get_thread_num());
    return 1;
#endif
    UNUSED(fi);
    print_warn("fat_cache_init() is not implemented! Don't provide the 'NO_FAT_CACHE'!");
    return 1;
}

int fat_cache_hload(fat_data_t* fi) {
#ifndef NO_FAT_CACHE
    if (!_fat) return 0;
    for (cluster_addr_t ca = 0; ca < fi->total_clusters; ca++) {
        cluster_val_t cls = read_fat(ca, fi);
        if (cls == FAT_CLUSTER_FREE) fatmap_set(ca);
        else fatmap_unset(ca);
    }

    return 1;
#endif
    UNUSED(fi);
    print_warn("fat_cache_hload() is not implemented! Don't provide the 'NO_FAT_CACHE'!");
    return 1;
}

int fat_cache_unload() {
#ifndef NO_FAT_CACHE
    if (!_fat || !THR_require_write(&_fat_lock, get_thread_num())) return 0;
    nft32_free_s(_fat);
    _fat = NULL;
    THR_release_write(&_fat_lock, get_thread_num());
    return 1;
#endif
    print_warn("fat_cache_unload() is not implemented! Don't provide the 'NO_FAT_CACHE'!");
    return 1;
}

int fat_repair(fat_data_t* fi) {
    for (cluster_addr_t ca = 0; ca < fi->total_clusters; ca++) {
        (void)read_fat(ca, fi);
    }

    return 1;
}

#ifndef NIFAT32_RO
static int __write_fat__(cluster_addr_t ca, cluster_status_t value, fat_data_t* fi, int fat) {
    cluster_offset_t fat_offset = ca * sizeof(cluster_val_t) * sizeof(encoded_t);
    sector_addr_t fat_sector = fi->sectors_padd + GET_FATSECTOR(fat, fi->total_sectors) + (fat_offset / fi->bytes_per_sector);
    
    decoded_t table_buffer[sizeof(cluster_val_t)] = { 0 };
    nft32_pack_memory((const byte_t*)&value, table_buffer, sizeof(cluster_val_t));
    if (
        !DSK_writeoff_sectors(fat_sector, fat_offset % fi->bytes_per_sector, (const unsigned char*)table_buffer, sizeof(table_buffer), 1)
    ) {
        print_error("Could not write new NiFAT32 cluster number to sector.");
        errors_register_error(WRITE_FAT_ERROR, fi);
        return 0;
    }

    return 1;    
} 

/* Move part which doesn't lock anything 'cause we have to write not only 
   in the write function, but also in the read function as well. In read 
   function we've locked the mutex, and can't lock it one more time. */
static inline int _write_fat_locked(cluster_addr_t ca, cluster_status_t value, fat_data_t* fi) {
    if (_fat) _fat[ca] = value;
    if (value == FAT_CLUSTER_FREE) fatmap_set(ca);
    else fatmap_unset(ca);
    int result = 1;
    for (int i = 0; i < fi->fat_count; i++) result = __write_fat__(ca, value, fi, i) && result;
    return result;
}
#endif

int write_fat(cluster_addr_t ca, cluster_status_t value, fat_data_t* fi) {
#ifndef NIFAT32_RO
    print_debug("write_fat(ca=%u, value=%u)", ca, value);
    if (ca < fi->ext_root_cluster || ca >= fi->total_clusters) {
        print_error("Can't write cluster! Wrong cluster address! %u", ca);
        errors_register_error(WRITE_FAT_ERROR, fi);
        return 0;
    }

    if (!THR_require_write(&_fat_lock, get_thread_num())) {
        print_error("Can't write-lock FAT!");
        errors_register_error(WRITE_FAT_ERROR, fi);
        return 0;
    }

    int result = _write_fat_locked(ca, value, fi);
    THR_release_write(&_fat_lock, get_thread_num());
    return result;
#endif
    UNUSED(ca, value, fi);
    return 1;
}

static cluster_val_t __read_fat__(cluster_addr_t ca, fat_data_t* fi, int fat) {
    cluster_offset_t fat_offset = ca * sizeof(cluster_val_t) * sizeof(encoded_t);
    sector_addr_t fat_sector = fi->sectors_padd + GET_FATSECTOR(fat, fi->total_sectors) + (fat_offset / fi->bytes_per_sector);
    
    encoded_t table_buffer[sizeof(cluster_val_t)] = { 0 };
    if (
        !DSK_readoff_sectors(fat_sector, fat_offset % fi->bytes_per_sector, (unsigned char*)table_buffer, sizeof(table_buffer), 1
    )) {
        print_error("Could not read sector that contains NiFAT32 table entry needed.");
        errors_register_error(READ_FAT_ERROR, fi);
        return FAT_CLUSTER_BAD;
    }

    cluster_val_t table_value = 0;
    nft32_unpack_memory(table_buffer, (byte_t*)&table_value, sizeof(cluster_val_t));
    return table_value & 0x0FFFFFFF;
}

cluster_val_t read_fat(cluster_addr_t ca, fat_data_t* fi) {
    print_debug("read_fat(ca=%u)", ca);
    if (ca < fi->ext_root_cluster || ca >= fi->total_clusters) {
        print_error("Can't read cluster! Wrong cluster address! %u", ca);
        errors_register_error(READ_FAT_ERROR, fi);
        return FAT_CLUSTER_BAD;
    }

    if (!THR_require_write(&_fat_lock, get_thread_num())) {
        print_error("Can't write-lock FAT!");
        errors_register_error(READ_FAT_ERROR, fi);
        return FAT_CLUSTER_BAD;
    }

    if (_fat && _fat[ca] != FAT_CLUSTER_BAD) {
        print_debug("cached read_fat(ca=%u) -> %u", ca, _fat[ca]);
        cluster_val_t cached = _fat[ca];
        THR_release_write(&_fat_lock, get_thread_num());
        return cached;
    }

    int wrong = -1;
    int val_freq = 0;
    cluster_val_t table_value = FAT_CLUSTER_BAD;
    for (int i = 0; i < fi->fat_count; i++) {
        cluster_val_t fat_val = __read_fat__(ca, fi, i);
        if (fat_val == table_value) val_freq++;
        else {
            val_freq--;
            wrong++;
        }

        if (val_freq < 0) {
            table_value = fat_val;
            val_freq = 0;
        }
    }

    if (_fat) _fat[ca] = table_value;
    if (table_value == FAT_CLUSTER_FREE) fatmap_set(ca);
    else fatmap_unset(ca);

#ifndef NIFAT32_RO
    if (wrong > 0) {
        print_warn("FAT wrong value at ca=%u. Fixing to val=%u...", ca, table_value);
        _write_fat_locked(ca, table_value, fi);
    }
#endif

    print_debug("read_fat(ca=%u) -> %u", ca, table_value);
    THR_release_write(&_fat_lock, get_thread_num());
    return table_value;
}
