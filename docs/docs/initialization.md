# Initialization

Include the public header:

```c
#include "nifat32.h"
```

NiFAT32 does not perform POSIX I/O itself. The platform supplies sector I/O, logging, and optionally memory callbacks.

```c
static int read_sector(
    sector_addr_t sector,
    sector_offset_t offset,
    unsigned char *buffer,
    int size
) {
    /* Read size bytes from sector * SECTOR_SIZE + offset. */
    return 1;
}

static int write_sector(
    sector_addr_t sector,
    sector_offset_t offset,
    const unsigned char *data,
    int size
) {
    /* Write size bytes to sector * SECTOR_SIZE + offset. */
    return 1;
}
```

Both callbacks return nonzero on success and zero on failure.

```c
#define SECTOR_SIZE 512
#define IMAGE_SIZE_BYTES (64u * 1024u * 1024u)

nifat32_params_t params = {
    .fat_cache        = CACHE,
    .bs_num           = 0,
    .bs_count         = 5,
    .ts               = IMAGE_SIZE_BYTES / SECTOR_SIZE,
    .jc               = 2,
    .ec               = 1,
    .disk_io          = {
        .read_sector  = read_sector,
        .write_sector = write_sector,
        .sector_size  = SECTOR_SIZE
    },
    .logg_io          = {
        .fd_fprintf   = NULL,
        .fd_vfprintf  = NULL
    },
    .mm_manager       = { DEFAULT_MM_MANAGER }
};

if (!NIFAT32_init(&params)) {
    /* Mount failed. */
}
```

## Mount parameters

| Field        | Meaning                                                                           |
| ---          | ---                                                                               |
| `fat_cache`  | Bit mask made from `NO_CACHE`, `CACHE`, `HARD_CACHE`, and `MAP_CACHE`             |
| `bs_num`     | First boot sector copy to try; normally `0`                                       |
| `bs_count`   | Number of boot sector copies created by `--bsbc`                                  |
| `ts`         | Total logical sectors in the image                                                |
| `jc`         | Journal copy count created by `--jc`                                              |
| `ec`         | Persistent error-storage copy count; use `0` to disable at runtime                |
| `ef`         | Entries offset which means it will enable presudo-random placement over the image |
| `disk_io`    | Sector read/write callbacks and logical sector size                               |
| `logg_io`    | Optional `fprintf`-like and `vfprintf`-like callbacks                             |
| `mm_manager` | Allocator callbacks or `DEFAULT_MM_MANAGER`                                       |

`NIFAT32_init()` increments `bs_num` while trying damaged boot sector copies, so the structure may be modified after a failed or recovered mount.

Cache modes must be combined with `CACHE`:

| Value | Behavior |
| --- | --- |
| `NO_CACHE` | Read FAT values from storage |
| `CACHE` | Allocate a lazy FAT cache |
| `CACHE \| HARD_CACHE` | Load the complete FAT into the cache during mount |
| `CACHE \| MAP_CACHE` | Allocate a free-cluster bitmap |
| `CACHE \| HARD_CACHE \| MAP_CACHE` | Enable both full loading and the bitmap |

The image geometry and mount parameters must agree. In particular, `bs_count`, `ts`, `jc`, `ec`, and `sector_size` must describe the formatted image. FAT count and sectors per cluster are read from the boot sector.

## Custom memory manager

The callback signatures are:

```c
static int mm_init(void);
static void *mm_alloc(unsigned long size);
static void mm_free(void *ptr);
```

Configure and build:

```c
nifat32_params_t params = {
    /* ... */
    .mm_manager = {
        .init   = mm_init,
        .malloc = mm_alloc,
        .free   = mm_free
    }
};
```

```bash
make NO_DEFAULT_MM_MANAGER=1
```

When using the built-in manager, `{ DEFAULT_MM_MANAGER }` selects its static buffer. Increase `ALLOC_BUFFER_SIZE` when FAT caching or directory indexes need more memory.
