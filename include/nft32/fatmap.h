/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Cluster allocation bitmap used to find and mark free clusters.

Dependencies:
    - std/mm.h - Filesystem memory manager.
    - std/str.h - Memory helpers.
    - std/null.h - NULL definition.
    - std/logging.h - Logging helpers.
    - std/threading.h - Bitmap locks.
    - nft32/errors.h - Error registration.
    - nft32/fatinfo.h - FAT filesystem metadata.
*/

#ifndef FATMAP_H_
#define FATMAP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <std/mm.h>
#include <std/str.h>
#include <std/null.h>
#include <std/logging.h>
#include <std/threading.h>
#include <nft32/errors.h>
#include <nft32/fatinfo.h>

#define BITMAP_NFREE             0x00000000
#define BITMAP_IS_FREE(val, pos) (!(((val) >> (pos)) & 1))
typedef unsigned int bitmap_val_t;

#define BITS_PER_WORD (sizeof(bitmap_val_t) * 8)

/*
Initialize the free-cluster bitmap.
[Thread-safe]
*/
int fatmap_init(fat_data_t* fi);

/*
Mark cluster as free in the bitmap.
[Thread-safe]
*/
int fatmap_set(unsigned int ca);

/*
Mark cluster as occupied in the bitmap.
[Thread-safe]
*/
int fatmap_unset(unsigned int ca);

/*
Find a free cluster range in the bitmap.
[Thread-safe]
*/
unsigned int fatmap_find_free(unsigned int offset, int size, fat_data_t* fi);

/*
Release the free-cluster bitmap.
[Thread-safe]
*/
int fatmap_unload();

#ifdef __cplusplus
}
#endif
#endif
