# Reference footprint

**Minimum read-only:** 6,767 B ROM · 856 B static RAM. Default RAM includes a 500 KiB allocator pool.

<!-- footprint:charts -->

## Measurement

Fedora x86-64 · GCC 15.2.1 · Binutils 2.45.1.

- **ROM:** `text + data`. **Static RAM:** `data + bss`; excludes stacks and external allocation.
- **External allocator:** `NO_DEFAULT_MM_MANAGER=1` removes the built-in pool.
- **Minimum read-only:** no mutations, caches, error reporting, entry validation or thread safety.
- Common flags: `-ffunction-sections -fdata-sections -Wl,--gc-sections`.

MCU sizes depend on the target toolchain.

## All 16 measurements

Complete flags per configuration; `-Os` and `-Oz` replace `-O2`.

<details>
<summary>Show exact byte counts and build flags</summary>

<!-- footprint:table -->

</details>

## Maximum profile

| Profile | Configuration | ROM | Static RAM | ELF file | Stripped ELF |
| --- | --- | ---: | ---: | ---: | ---: |
| Maximum feature/debug profile | `-O0 -g3`, all `*_LOGS=1`, default features and numeric limits | **63.9 KiB** | **505.2 KiB** | 164.6 KiB | 71.1 KiB |

Minimum measured build:

```bash
make shared \
  BUILD_DIR=builds/minimum \
  CFLAGS="-Oz -flto -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-plt" \
  LDFLAGS="-flto -Wl,--gc-sections -Wl,--version-script=nifat32.exports -Wl,--build-id=none -Wl,-z,noseparate-code" \
  NIFAT32_RO=1 \
  NO_HEAP=1 \
  NO_DEFAULT_MM_MANAGER=1 \
  NIFAT32_NO_ECACHE=1 \
  NO_FAT_CACHE=1 \
  NO_FAT_MAP=1 \
  NIFAT32_NO_ERROR=1 \
  NO_ENTRY_VALIDATION=1 \
  NO_THREADSAFE=1 \
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

## On a chip

[Filesystem builder](../builder.md): STM32, ESP32 and ATmega flash budgets and copy placement. The spatial map models placement; it does not represent measured silicon coordinates.
