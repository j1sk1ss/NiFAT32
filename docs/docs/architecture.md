# Architecture

NiFAT32 looks like FAT32 from the API side but is not wire-compatible. Every metadata structure is stored in redundant copies, spread across the whole volume, encoded, and checksummed, so a single-event upset (SEU) or a local multiple-event upset (MEU/scratch) corrupts at most one or a few copies instead of the structure.

## On-disk layout

The image is a sequence of 512-byte sectors. Apart from a small fixed prefix, NiFAT32 does not place its structures in fixed blocks: the boot sector copies, FAT copies, journal areas, and error-storage areas are each placed at a pseudo-random sector chosen by a deterministic 32-bit mixer, so different copies of the same structure land far apart on the media.

```
| gap |  cluster gap (no data clusters)              | data clusters, root dir is cluster 2 ... |
| 0-1 |  32 .. 32 + fc*fatsize                        |  32 + fc*fatsize .. ts-1                     |
```

On top of this map the formatter scatters:

| Structure | Copies | Placement | On-disk size |
| --- | --- | --- | --- |
| Boot sector | `bsbc` | sector `2 + mix32(i + BOOT_SEED) % (ts - 2)` | 1 sector (encoded structure, zero-padded) |
| FAT | `fc` | sector `55 + mix32(i + FAT_SEED) % (ts - 23)` | `2 * fat_size` sectors (Hamming-encoded table) |
| Journal | `jc` | sector `2 + mix32(i + JOURNAL_SEED) % (ts - 2)` | `spc` sectors |
| Error storage | `ec` | sector `2 + mix32(i + ERRORS_SEED) % (ts - 2)` | `spc` sectors |

`mix32` is a small deterministic mixer (shift-multiply-shift), each structure type has its own seed, and the placement depends only on the copy index and the total sector count. The same formulas run in the formatter and in the library, so the mount never searches for structures — it computes where they are.

Cluster numbering keeps the FAT32 convention: cluster 0 and 1 are reserved, the root directory lives at cluster 2, and cluster `n` occupies sectors `(n - 2) * spc + first_data_sector`, where `first_data_sector = reserved_sector_count + fc * fat_size` (`reserved_sector_count` is 32 in the reference geometry). The FAT table itself covers all `total_clusters` entries, and its size is solved iteratively so the table fits its own allocated space.

File cluster chains are also started at pseudo-random offsets within the free space, so a scratch that destroys one neighborhood does not take out all clusters of the hottest files.

## Protection layers

| Layer | What it protects | How |
| --- | --- | --- |
| Hamming(15,11) | Every stored byte of boot sectors, FAT, journals, directory entries, error records | Each byte is expanded to a 15-bit word that corrects a single flipped bit; decoding happens before any checksum |
| Murmur3 checksum | Boot sector (base + extended section), directory entries, journal entries | Recomputed on read; a mismatch means the copy is untrusted |
| Redundant copies | Boot sectors (`bsbc`), FAT (`fc`), journals (`jc`), error records (`ec`) | Copies are spread by the mixer; damaged copies are replaced from good ones |
| Majority voting | Every FAT entry read, every journal entry read | The value from the majority of copies wins; on disagreement the winner is written back to all copies |
| Journaling (WAL) | Directory entry add/edit/delete | The operation is recorded in all journal copies before the directory cluster is modified; on mount, unsolved entries are replayed and then cleared |
| Error ring | Post-mortem diagnostics | Recovered faults are registered as error codes that the host can consume with `NIFAT32_get_last_error()` |

The layers are ordered so a radiation event meets several independent obstacles: a bit flipped inside one copy is corrected by the Hamming word; if it is not, the copy checksum fails and the copy loses the vote; the vote is repaired from the other copies; a scratch wide enough to hit several copies still has to hit a strict majority of *all* copies of *every* structure, which the pseudo-random placement makes unlikely.

## Mount sequence

```
1. Read boot sector copy #bs_num at its computed sector
2. Decode (Hamming), verify both Murmur3 checksums
3. Checksum mismatch  ->  register CHECKSUM_CHECK_ERROR, bs_num++, retry (up to bs_count)
4. If bs_num > 0      ->  rewrite the good encoded boot sector to all other copies
5. Compute geometry from the boot sector (spc, fat_size, fat_count, total sectors)
6. Optionally load the FAT cache (lazy, full load, free-cluster bitmap)
7. Replay unsolved journal entries from all journal copies
```

## Write sequence (file)

```
1. Lookup the path (name hashes + optional entry cache)
2. Allocate clusters (pseudo-random start offset within free space)
3. write_fat: update the FAT entry in ALL fc copies (plus the cache and free-cluster bitmap)
4. Journal: record the directory entry operation in all jc journal copies
5. Write the encoded directory entry (with checksum) into the directory cluster
6. Journal: clear the slot (NO_OP) in all copies
7. Write the file data into the allocated clusters
```

The journal slot is written before the directory cluster, so a power loss between the two steps leaves an unsolved entry that the next mount replays. The journal entry carries the full squeezed directory entry, not just the operation, which is why a replay is a plain rewrite.

## Read sequence (FAT entry)

```
1. Cache hit and value is not BAD -> return the cached value
2. Otherwise read the entry from all fc FAT copies
3. Hamming-decode each copy
4. Majority vote over the decoded values
5. Any disagreement -> write the winning value back to all fc copies
6. Cache the result
```

The same voting routine is used for journal entries, compared by checksum.

## Persistent error ring

With `ec > 0` the library keeps `ec` pseudo-randomly placed areas of `spc` sectors and registers error codes there as they happen. The host pulls them out one at a time:

```c
error_code_t error = NIFAT32_get_last_error(); // -1 when the ring is empty
```

The ring is the only place where a fault is recorded *before* it is repaired, so the host can count and classify upsets even when the filesystem keeps working.
