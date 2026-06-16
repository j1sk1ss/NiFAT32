# NiFAT32

NiFAT32 is a FAT32-like filesystem designed to tolerate single-event and multiple-event upsets. It protects boot sectors, FAT entries, directory entries, journals, and persistent error records with redundant copies, checksums, voting, and Hamming encoding. </br>
P.S.: *NiFAT32 is not wire-compatible with a regular FAT32 implementation. Images must be accessed through this library or compatible firmware.*

## Reference Footprint

The following measurements were produced on Fedora Linux x86-64 with GCC 15.2.1 and GNU Binutils 2.45.1.

### Flags

P.S.: *All rows use `-ffunction-sections -fdata-sections` and `-Wl,--gc-sections`*.

| Flags | ROM | ROM change | Static RAM | RAM change | Stripped ELF |
| --- | ---: | ---: | ---: | ---: | ---: |
| `-O2` | 41.2 KiB | - | 505.2 KiB | - | 47.1 KiB |
| + `NIFAT32_RO=1` | 32.5 KiB | -8.7 KiB | 505.1 KiB | -0.1 KiB | 39.0 KiB |
| + `NO_HEAP=1` | 32.5 KiB | 0.0 KiB | 505.1 KiB | 0.0 KiB | 39.0 KiB |
| + `NO_DEFAULT_MM_MANAGER=1` | 32.0 KiB | -0.5 KiB | 5.1 KiB | -500.0 KiB | 39.0 KiB |
| + `NIFAT32_NO_ERROR=1` | 30.8 KiB | -1.2 KiB | 5.0 KiB | -0.1 KiB | 39.0 KiB |
| + `NO_ENTRY_VALIDATION=1` | 30.7 KiB | -0.1 KiB | 5.0 KiB | 0.0 KiB | 39.0 KiB |
| + `CONTENT_TABLE_SIZE=8 IO_THREADS_MAX=4` | 30.4 KiB | -0.3 KiB | 1.8 KiB | -3.2 KiB | 38.7 KiB |
| + `CONTENT_TABLE_SIZE=1 IO_THREADS_MAX=1` | 29.9 KiB | -0.5 KiB | 1.3 KiB | -0.5 KiB | 38.7 KiB |
| + `NIFAT32_NO_ECACHE=1 NO_FAT_CACHE=1 NO_FAT_MAP=1` | 24.0 KiB | -5.8 KiB | 1.3 KiB | less than -0.1 KiB | 30.6 KiB |
| `-Os` | 20.7 KiB | -3.4 KiB | 1.2 KiB | less than -0.1 KiB | 26.6 KiB |
| `-Oz` | 20.4 KiB | -0.3 KiB | 1.2 KiB | 0.0 KiB | 26.6 KiB |
| + `-flto` | **20.3 KiB** | less than -0.1 KiB | **1.3 KiB** | less than +0.1 KiB | 26.6 KiB |

The exact minimum measured values are 20,809 bytes of `text + data` and 1,308 bytes of `data + bss`. LTO reduced ROM by another 36 bytes, although the rounded KiB value remains almost unchanged.

### Maximum Profile

For comparison, enabling every logging group with debug-friendly optimization produces:

| Profile | Configuration | ROM | Static RAM | ELF file | Stripped ELF |
| --- | --- | ---: | ---: | ---: | ---: |
| Maximum feature/debug profile | `-O0 -g3`, all `*_LOGS=1`, default features and numeric limits | **63.9 KiB** | **505.2 KiB** | 164.6 KiB | 71.1 KiB |

Minimum measured build:

```bash
make shared \
  BUILD_DIR=builds/minimum \
  CFLAGS="-Oz -flto -ffunction-sections -fdata-sections" \
  LDFLAGS="-flto -Wl,--gc-sections" \
  NIFAT32_RO=1 \
  NO_HEAP=1 \
  NO_DEFAULT_MM_MANAGER=1 \
  NIFAT32_NO_ECACHE=1 \
  NO_FAT_CACHE=1 \
  NO_FAT_MAP=1 \
  NIFAT32_NO_ERROR=1 \
  NO_ENTRY_VALIDATION=1 \
  CONTENT_TABLE_SIZE=1 \
  IO_THREADS_MAX=1
```

Maximum feature/debug build:

```bash
make shared \
  BUILD_DIR=builds/maximum \
  CFLAGS="-O0 -g3" \
  ERROR_LOGS=1 WARN_LOGS=1 INFO_LOGS=1 DEBUG_LOGS=1 \
  IO_LOGS=1 MEM_LOGS=1 LOGGING_LOGS=1 SPECIAL_LOGS=1
```

These numbers do not include thread stacks, temporary stack buffers, or memory supplied by a custom allocator. With the built-in allocator, its entire pool is counted in `.bss` even when only part of it is used at runtime. The default static RAM is therefore dominated by the 500 KiB `ALLOC_BUFFER_SIZE`. For the current root build, pass `NIFAT32_NO_ECACHE=1 NO_FAT_CACHE=1 NO_FAT_MAP=1` explicitly to disable heap-backed caches, and use a smaller `ALLOC_BUFFER_SIZE` or `NO_DEFAULT_MM_MANAGER=1` to remove the large built-in pool.

## Building

The root Makefile tracks header dependencies and rebuilds objects when build options change.

```bash
make           # builds/nifat32.so
make static    # builds/nifat32.a
make unix      # builds/unix_nifat32
make formatter # formatter/formatter
make tools     # formatter and Unix utility
make clean     # remove root build artifacts
make distclean # also clean the formatter
make help
```

The usual toolchain variables can be overridden:

```bash
make CC=clang CFLAGS="-O3" BUILD_DIR=out
```

`CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and `LDLIBS` may also be supplied or extended by the caller.

### Build Options

Boolean Make variables accept `0` or `1` and default to `0`.

| Make variable | C macro | Effect |
| --- | --- | --- |
| `DEBUG` | - | Adds `-O0 -g3` |
| `NIFAT32_RO` | `NIFAT32_RO` | Compiles storage mutations as successful no-ops |
| `NO_HEAP` | `NO_HEAP` | Requests entry-cache, FAT-cache, FAT-map, and default-allocator disabling through header macros; pass the explicit flags for the current multi-file build |
| `NO_DEFAULT_MM_MANAGER` | `NO_DEFAULT_MM_MANAGER` | Removes the built-in static allocator; callbacks must be supplied |
| `NIFAT32_NO_ERROR` | `NIFAT32_NO_ERROR` | Disables persistent error storage |
| `NIFAT32_NO_ECACHE` | `NIFAT32_NO_ECACHE` | Disables directory entry indexing |
| `NO_FAT_CACHE` | `NO_FAT_CACHE` | Disables the in-memory FAT value cache |
| `NO_FAT_MAP` | `NO_FAT_MAP` | Disables the free-cluster bitmap |
| `NO_ENTRY_VALIDATION` | `NO_ENTRY_VALIDATION` | Skips directory-entry checksum validation |
| - | `NO_THREADSAFE` | Make the FS not thread safe (exclude related code from the final binary) |

Numeric configuration variables:

| Make variable | C macro | Default | Effect |
| --- | --- | --- | --- |
| `ALLOC_BUFFER_SIZE` | `ALLOC_BUFFER_SIZE` | `512000` | Built-in allocator buffer size in bytes |
| `CONTENT_TABLE_SIZE` | `CONTENT_TABLE_SIZE` | `50` | Maximum number of simultaneously open content entries |
| `IO_THREADS_MAX` | `IO_THREADS_MAX` | `25` | Maximum number of tracked I/O lock areas |

`NO_HEAP=1` defines `NIFAT32_NO_ECACHE`, `NO_FAT_CACHE`, and `NO_FAT_MAP` inside `nifat32.h`. `NON_DEFAULT_MM_MANAGER` is still accepted by the source as a legacy alias, but new builds should use `NO_DEFAULT_MM_MANAGER`.

Examples:

```bash
make NIFAT32_RO=1 NO_HEAP=1
make ALLOC_BUFFER_SIZE=1048576 CONTENT_TABLE_SIZE=100
make NO_DEFAULT_MM_MANAGER=1
```

### Logging Options

All log groups are disabled by default. Each Make variable enables the corresponding compile-time macro.

| Make variable | C macro | Log category |
| --- | --- | --- |
| `ERROR_LOGS` | `ERROR_LOGS` | Errors |
| `WARN_LOGS` | `WARNING_LOGS` | Warnings |
| `INFO_LOGS` | `INFO_LOGS` | General information |
| `DEBUG_LOGS` | `DEBUG_LOGS` | Debug messages |
| `IO_LOGS` | `IO_OPERATION_LOGS` | Disk operations |
| `MEM_LOGS` | `MEM_OPERATION_LOGS` | Allocator operations |
| `LOGGING_LOGS` | `LOGGING_LOGS` | Public API calls |
| `SPECIAL_LOGS` | `SPECIAL_LOGS` | Special-purpose diagnostics |

```bash
make ERROR_LOGS=1 WARN_LOGS=1 INFO_LOGS=1
make DEBUG=1 DEBUG_LOGS=1 IO_LOGS=1
```

Enabled logging also requires valid logging callbacks in
`nifat32_params_t`. Passing `NULL` callbacks disables output at runtime.

## Creating an Image

Build and run the formatter:

```bash
make formatter
./formatter/formatter -o nifat32.img
```

Create a 128 MB image and populate it from `data/`:

```bash
./formatter/formatter \
  -o nifat32.img \
  -s data \
  --volume-size 128 \
  --spc 8 \
  --fc 4 \
  --bsbc 5 \
  --b-bsbc 0 \
  --jc 2
```

| Option | Description | Default |
| --- | --- | --- |
| `-o PATH` | Output image; existing files are overwritten | Required |
| `-s PATH` | Source directory copied recursively into the image | Empty image |
| `--volume-size N` | Image size in MB | `64` |
| `--spc N` | Sectors per cluster | `8` |
| `--fc N` | FAT copy count | `4` |
| `--bsbc N` | Boot sector copy count | `5` |
| `--b-bsbc N` | Deliberately damaged boot sector copies for tests | `0` |
| `--jc N` | Journal sector count | `2` |

The formatter creates a raw filesystem starting at sector zero, without an
MBR or GPT. See [`formatter/README.md`](formatter/README.md) for SD card
instructions and safety notes.

## Initialization

Include the public header:

```c
#include "nifat32.h"
```

NiFAT32 does not perform POSIX I/O itself. The platform supplies sector I/O,
logging, and optionally memory callbacks.

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
    .fat_cache = CACHE,
    .bs_num = 0,
    .bs_count = 5,
    .ts = IMAGE_SIZE_BYTES / SECTOR_SIZE,
    .jc = 2,
    .ec = 1,
    .disk_io = {
        .read_sector = read_sector,
        .write_sector = write_sector,
        .sector_size = SECTOR_SIZE
    },
    .logg_io = {
        .fd_fprintf = NULL,
        .fd_vfprintf = NULL
    },
    .mm_manager = { DEFAULT_MM_MANAGER }
};

if (!NIFAT32_init(&params)) {
    /* Mount failed. */
}
```

### Mount Parameters

| Field | Meaning |
| --- | --- |
| `fat_cache` | Bit mask made from `NO_CACHE`, `CACHE`, `HARD_CACHE`, and `MAP_CACHE` |
| `bs_num` | First boot sector copy to try; normally `0` |
| `bs_count` | Number of boot sector copies created by `--bsbc` |
| `ts` | Total logical sectors in the image |
| `jc` | Journal copy count created by `--jc` |
| `ec` | Persistent error-storage copy count; use `0` to disable at runtime |
| `disk_io` | Sector read/write callbacks and logical sector size |
| `logg_io` | Optional `fprintf`-like and `vfprintf`-like callbacks |
| `mm_manager` | Allocator callbacks or `DEFAULT_MM_MANAGER` |

`NIFAT32_init()` increments `bs_num` while trying damaged boot sector copies, so the structure may be modified after a failed or recovered mount.

Cache modes must be combined with `CACHE`:

| Value | Behavior |
| --- | --- |
| `NO_CACHE` | Read FAT values from storage |
| `CACHE` | Allocate a lazy FAT cache |
| `CACHE \| HARD_CACHE` | Load the complete FAT into the cache during mount |
| `CACHE \| MAP_CACHE` | Allocate a free-cluster bitmap |
| `CACHE \| HARD_CACHE \| MAP_CACHE` | Enable both full loading and the bitmap |

The image geometry and mount parameters must agree. In particular,
`bs_count`, `ts`, `jc`, `ec`, and `sector_size` must describe the formatted
image. FAT count and sectors per cluster are read from the boot sector.

### Custom Memory Manager

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
        .init = mm_init,
        .malloc = mm_alloc,
        .free = mm_free
    }
};
```

```bash
make NO_DEFAULT_MM_MANAGER=1
```

When using the built-in manager, `{ DEFAULT_MM_MANAGER }` selects its static buffer. Increase `ALLOC_BUFFER_SIZE` when FAT caching or directory indexes need more memory.

## Names and Open Modes

NiFAT32 stores and accepts names in uppercase FAT 8.3 form. Convert regular
names and paths before passing them to the public API.

```c
char path[128] = { 0 };
nft32_path_to_fatnames("root/dir/hello.txt", path);
/* path is now ROOT/DIR/HELLO   TXT */
```

Available helpers from `std/fatname.h`:

| Function | Purpose |
| --- | --- |
| `nft32_name_to_fatname` | Convert `hello.txt` to `HELLO   TXT` |
| `nft32_fatname_to_name` | Convert `HELLO   TXT` to a regular name |
| `nft32_path_to_fatnames` | Convert a path to the NiFAT32 path form |
| `nft32_extract_name` | Extract the final path component |
| `unpack_83_name` | Split an 11-byte FAT name into name and extension |

Open modes:

| Flag | Meaning |
| --- | --- |
| `R_MODE` | Permit reads |
| `W_MODE` | Permit writes |
| `CR_MODE` | Create missing path components |
| `NO_TARGET` | No explicit type for the final component |
| `FILE_TARGET` | Create the final component as a file |
| `DIR_TARGET` | Create the final component as a directory |
| `DF_MODE` | `MODE(R_MODE \| W_MODE, NO_TARGET)` |

Use `MODE(access_flags, target)` to pack access and target flags:

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("root/hello.txt", fat_path);

ci_t file = NIFAT32_open_content(
    NO_RCI,
    fat_path,
    MODE(R_MODE | W_MODE | CR_MODE, FILE_TARGET)
);
```

`NO_RCI` starts lookup at the filesystem root. Passing an open directory index instead starts lookup relative to that directory. A `NULL` path opens the root directory.

## Public API

All public functions are declared in `nifat32.h`.

### Check if content exists

`NIFAT32_content_exists()` searches from the filesystem root and returns `1` when the path exists.

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("data/config.bin", fat_path);

if (NIFAT32_content_exists(fat_path)) {
    // The entry exists and can be opened.
}
```

### Open content

Open an entry with the required access mode. A nonnegative value is a valid content index.

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("data/config.bin", fat_path);

ci_t file = NIFAT32_open_content(
    NO_RCI,
    fat_path,
    MODE(R_MODE | W_MODE, FILE_TARGET)
);

if (file < 0) {
    // -1 means that the content table is full.
    // -2 means that the path was not found.
}
```

Pass an open directory as `rci` to search relative to it:

```c
char directory_name[12] = { 0 };
char file_name[12] = { 0 };
nft32_name_to_fatname("data", directory_name);
nft32_name_to_fatname("config.bin", file_name);

ci_t directory = NIFAT32_open_content(NO_RCI, directory_name, DF_MODE);

if (directory >= 0) {
    ci_t file = NIFAT32_open_content(directory, file_name, DF_MODE);
    if (file >= 0) NIFAT32_close_content(file);
    NIFAT32_close_content(directory);
}
```

### Create content

Add `CR_MODE` to create missing path components. Intermediate components are created as directories.

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("logs/boot.txt", fat_path);

ci_t file = NIFAT32_open_content(
    NO_RCI,
    fat_path,
    MODE(R_MODE | W_MODE | CR_MODE, FILE_TARGET)
);

if (file >= 0) {
    // logs/boot.txt now exists and is open.
}
```

Use `NIFAT32_put_content()` when cluster reservation is required:

```c
ci_t root = NIFAT32_open_content(NO_RCI, NULL, DF_MODE);
cinfo_t info = {
    .full_name = "LOG     BIN",
    .type = STAT_FILE
};

if (root >= 0) {
    NIFAT32_put_content(root, &info, 4, NO_SEED); // Preallocate four clusters.
    NIFAT32_close_content(root);
}
```

`NO_RESERVE` is `1`. Creation through `NIFAT32_open_content()` is more convenient for paths, but it does not expose cluster reservation.

### Read content

The content must be opened with `R_MODE`. The function returns the number of bytes copied to the buffer.

```c
char buffer[64] = { 0 };

int read_size = NIFAT32_read_content2buffer(
    file,
    0, // Byte offset in the content.
    (buffer_t)buffer,
    sizeof(buffer)
);

if (read_size > 0) {
    // buffer[0..read_size-1] contains the data.
}
```

Reading a directory returns its raw encoded directory data.

### Write content

The content must be opened with `W_MODE`. The function returns the number of bytes written.

```c
const char message[] = "System started\n";

int written = NIFAT32_write_buffer2content(
    file,
    0, // Byte offset in the content.
    (const_buffer_t)message,
    sizeof(message) - 1
);

if (written != (int)(sizeof(message) - 1)) {
    // The complete message was not written.
}
```

The current write implementation grows cluster chains but does not automatically update file-size metadata. Use `NIFAT32_change_meta()` or `NIFAT32_truncate_content()` when the stored size must be updated.

### Get content information

`NIFAT32_stat_content()` fills a `cinfo_t` structure with the entry name and type.

```c
cinfo_t stat = { 0 };

if (NIFAT32_stat_content(file, &stat)) {
    // stat.full_name contains the FAT 8.3 name.
    // stat.type is STAT_FILE or STAT_DIR.
}
```

| Field | Meaning |
| --- | --- |
| `full_name[12]` | 11-byte FAT 8.3 name plus storage for a terminator |
| `name[8]` | Base name |
| `extention[4]` | Extension; the public field retains this spelling |
| `size` | File size metadata |
| `type` | `STAT_FILE` or `STAT_DIR` |

The current `NIFAT32_stat_content()` implementation does not populate `size` for files. Do not read that field after `stat` until the implementation is corrected.

### Change content metadata

`NIFAT32_change_meta()` changes directory entry metadata. It does not move or resize the cluster chain.

```c
cinfo_t info = {
    .full_name = "CONFIG  BAK",
    .size = 128,
    .type = STAT_FILE
};

if (!NIFAT32_change_meta(file, &info)) {
    // Metadata update failed.
}
```

### Truncate content

Truncation changes the occupied cluster chain and stored file size. The content must be open with `W_MODE`.

```c
if (NIFAT32_truncate_content(file, 0, 128)) {
    // Keep 128 bytes starting from offset zero.
}
```

### Index a directory

Index frequently searched directories to speed up repeated lookups.

```c
char directory_name[12] = { 0 };
char file_name[12] = { 0 };
nft32_name_to_fatname("data", directory_name);
nft32_name_to_fatname("config.bin", file_name);

ci_t directory = NIFAT32_open_content(NO_RCI, directory_name, DF_MODE);

if (directory >= 0) {
    NIFAT32_index_content(directory);

    // Relative lookups can now use the directory index.
    ci_t file = NIFAT32_open_content(directory, file_name, DF_MODE);
    if (file >= 0) NIFAT32_close_content(file);
    NIFAT32_close_content(directory);
}
```

Indexing requires allocator support and has no useful effect when compiled with `NIFAT32_NO_ECACHE`.

### Copy content

The destination must already exist as a placeholder:

| Copy type | Behavior |
| --- | --- |
| `DEEP_COPY` | Allocate a new cluster chain and recursively duplicate directory data |
| `SHALLOW_COPY` | Replace the destination with a link to the source cluster chain |

```c
char source_path[64] = { 0 };
char copy_path[64] = { 0 };
nft32_path_to_fatnames("data/config.bin", source_path);
nft32_path_to_fatnames("backup.bin", copy_path);

ci_t source = NIFAT32_open_content(NO_RCI, source_path, DF_MODE);
ci_t copy = NIFAT32_open_content(
    NO_RCI,
    copy_path,
    MODE(R_MODE | W_MODE | CR_MODE, FILE_TARGET)
);

if (source >= 0 && copy >= 0) {
    NIFAT32_copy_content(source, copy, DEEP_COPY);
}

if (source >= 0) NIFAT32_close_content(source);
if (copy >= 0) NIFAT32_close_content(copy);
```

Copying deallocates the destination's previous data. A shallow copy must not be placed in the same directory under a conflicting name.

### Delete content

Deleting a directory also removes its children recursively. The supplied content index is closed by the function.

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("backup.bin", fat_path);

ci_t file = NIFAT32_open_content(NO_RCI, fat_path, DF_MODE);

if (file >= 0) {
    NIFAT32_delete_content(file);
    // Do not close file: NIFAT32_delete_content() already did it.
}
```

### Repair boot sectors

`NIFAT32_repair_bootsectors()` rebuilds every boot sector copy from the currently mounted geometry.

```c
NIFAT32_repair_bootsectors(); // Rewrite all boot sector copies.
```

### Repair content

`NIFAT32_repair_content()` reads, corrects and rewrites directory entries. Pass a nonzero second argument to visit subdirectories recursively.

```c
ci_t root = NIFAT32_open_content(NO_RCI, NULL, DF_MODE);

if (root >= 0) {
    NIFAT32_repair_content(root, 1); // Recursive repair.
    NIFAT32_close_content(root);
}
```

### Get filesystem information

`NIFAT32_get_fs_data()` copies the currently mounted geometry.

```c
fat_data_t fs = NIFAT32_get_fs_data();
```

### Get the last error

Persistent errors are consumed from the error ring one at a time.

```c
error_code_t error = NIFAT32_get_last_error();

if (error >= 0) {
    // Handle the registered error code.
}
```

The function returns `-1` when no error is available. With `NIFAT32_NO_ERROR=1`, registration is disabled and the function returns `NO_ERROR`.

### Close content

Close every successfully opened content index when it is no longer needed.

```c
if (file >= 0) {
    NIFAT32_close_content(file);
}
```

### Unload the filesystem

Call `NIFAT32_unload()` after all work is complete. It releases the FAT cache and destroys the content table.

```c
NIFAT32_unload();
```

### Read-Only Builds

With `NIFAT32_RO=1`, reads, lookup, stat, indexing, and lifecycle operations remain available. Storage-mutating paths are compiled as no-ops. In the current API these no-op functions generally return success, so callers must not use the return value to infer that persistent data changed.

Affected operations include write, truncate, metadata changes, creation, copy, deletion, journal writes, FAT writes, and repair writes.

## Unix Utility

Build:

```bash
make unix
```

Run:

```bash
./builds/unix_nifat32 nifat32.img 64 512 5 2
```

Arguments are image path, image size in MB, sector size, boot sector copy count, and journal count. Supported commands include `cd`, `ls`, `mkdir`, `mkfile`, `read`, `write`, `trunc`, `cp`, `mv`, `rm`, `frename`, `rs`, `ws`, and `get_le`.

## Testing

The test runner can build a fresh image and execute the C test suite:

```bash
python3 test/nifat32_tests.py \
  --new-image \
  --formatter formatter \
  --tests-folder test \
  --root-folder . \
  --clean \
  --test-type default
```

Fault-injection tests:

```bash
python3 test/nifat32_tests.py \
  --new-image \
  --formatter formatter \
  --tests-folder test \
  --root-folder . \
  --clean \
  --test-type bitflip \
  --injector-scenario test/injector_scenario.txt
```

See [`test/README.md`](test/README.md) for campaign and collector options.
