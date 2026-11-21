# Getting started

The root Makefile builds the shared library, the static library, the Unix utility, the formatter, and the test systems. It tracks header dependencies and rebuilds objects when build options change.

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

## Build options

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
| - | `NO_HAMMING` | Exclude all hamming-code related logic |
| - | `ENTRY_WITH_TIME` | Adds timestamp fields to directory entries; this changes the `create_entry` function signature and increases the size of each entry |

`ENTRY_WITH_TIME` is a C macro-only option. It stores FAT-style timestamp fields in each `directory_entry_t`, so fewer entries fit in one decoded directory cluster. Callers must also pass timestamp values to `create_entry`.

For instance, this is the signature without this flag:

```c
create_entry(fullname, is_dir, first_cluster, file_size, entry);
```

And this is with `ENTRY_WITH_TIME`:

```c
create_entry(
    fullname,
    is_dir,
    first_cluster,
    file_size,
    second,
    centisecond,
    hour,
    minute,
    year,
    mounth,
    day,
    entry
);
```

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

## Logging options

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

Enabled logging also requires valid logging callbacks in `nifat32_params_t`. Passing `NULL` callbacks disables output at runtime.
