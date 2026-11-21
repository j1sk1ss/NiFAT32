# Public API

All public functions are declared in `nifat32.h`.

## Names and open modes

NiFAT32 stores and accepts names in uppercase FAT 8.3 form. Convert regular names and paths before passing them to the public API.

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
| `nft32_unpack_83_name` | Split an 11-byte FAT name into name and extension |

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

## Check if content exists

`NIFAT32_content_exists()` searches from the filesystem root and returns `1` when the path exists.

```c
char fat_path[64] = { 0 };
nft32_path_to_fatnames("data/config.bin", fat_path);

if (NIFAT32_content_exists(fat_path)) {
  // The entry exists and can be opened.
}
```

## Open content

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

## Create content

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
  NIFAT32_put_content(root, &info, 4); // Preallocate four clusters.
  NIFAT32_close_content(root);
}
```

`NO_RESERVE` is `1`. Creation through `NIFAT32_open_content()` is more convenient for paths, but it does not expose cluster reservation.

## Read content

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

## Write content

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

## Get content information

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

## Change content metadata

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

## Truncate content

Truncation changes the occupied cluster chain and stored file size. The content must be open with `W_MODE`.

```c
if (NIFAT32_truncate_content(file, 0, 128)) {
  // Keep 128 bytes starting from offset zero.
}
```

## Index a directory

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

## Copy content

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

## Delete content

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

## Repair boot sectors

`NIFAT32_repair_bootsectors()` rebuilds every boot sector copy from the currently mounted geometry.

```c
NIFAT32_repair_bootsectors(); // Rewrite all boot sector copies.
```

## Repair content

`NIFAT32_repair_content()` reads, corrects and rewrites directory entries. Pass a nonzero second argument to visit subdirectories recursively.

```c
ci_t root = NIFAT32_open_content(NO_RCI, NULL, DF_MODE);

if (root >= 0) {
  NIFAT32_repair_content(root, 1); // Recursive repair.
  NIFAT32_close_content(root);
}
```

## Get filesystem information

`NIFAT32_get_fs_data()` copies the currently mounted geometry.

```c
fat_data_t fs = NIFAT32_get_fs_data();
```

## Get the last error

Persistent errors are consumed from the error ring one at a time.

```c
error_code_t error = NIFAT32_get_last_error();

if (error >= 0) {
  // Handle the registered error code.
}
```

The function returns `-1` when no error is available. With `NIFAT32_NO_ERROR=1`, registration is disabled and the function returns `NO_ERROR`.

## Close content

Close every successfully opened content index when it is no longer needed.

```c
if (file >= 0) {
  NIFAT32_close_content(file);
}
```

## Unload the filesystem

Call `NIFAT32_unload()` after all work is complete. It releases the FAT cache and destroys the content table.

```c
NIFAT32_unload();
```

## Read-only builds

With `NIFAT32_RO=1`, reads, lookup, stat, indexing, and lifecycle operations remain available. Storage-mutating paths are compiled as no-ops. In the current API these no-op functions generally return success, so callers must not use the return value to infer that persistent data changed.

Affected operations include write, truncate, metadata changes, creation, copy, deletion, journal writes, FAT writes, and repair writes.
