# NiFAT32 — Built for the unexpected

NiFAT32 is a FAT32-like filesystem designed to tolerate single-event and multiple-event upsets. It protects metadata with scattered redundant copies, Hamming encoding, checksums, majority voting, and write-ahead journaling.

- [Get started](docs/getting-started.md)
- [Understand the architecture](docs/architecture.md)
- [Explore the public API](docs/api.md)
- [Inspect the measured footprint](docs/footprint.md)
- [Try the filesystem builder](builder.md)
- [Follow the interactive walkthroughs](animations.md)

## Quick start

```bash
make tools
./formatter/formatter -o nifat32.img
./builds/unix_nifat32 nifat32.img 64 512 5 2
```

The default volume is 64 MB, with 8 sectors per cluster, 4 FAT copies, 5 boot sector copies, and 2 journal copies.

NiFAT32 is not wire-compatible with regular FAT32. Access images through this library or compatible firmware.

For website authoring and preview instructions, see [README.md](README.md).
