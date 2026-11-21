# Creating an image

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

The formatter creates a raw filesystem starting at sector zero, without an MBR or GPT. See [`formatter/README.md`](../../formatter/README.md) for SD card instructions and safety notes.

## Choosing the geometry

The geometry is a trade-off between redundancy and capacity:

| Parameter | Bigger means | Cost |
| --- | --- | --- |
| `--volume-size` | More user data | More flash on the chip |
| `--fc` | More votes for every FAT entry | Each extra FAT copy costs `2 * fat_size` sectors of the whole volume |
| `--bsbc` | More chances to find a good boot sector | 1 sector per copy plus the repair writes at mount |
| `--jc` | More votes for journal entries | `spc` sectors per copy |
| `--spc` | Bigger clusters, fewer FAT entries per MB | Higher internal fragmentation |

The FAT copy count is the most expensive redundancy: with `--spc 8` a 64 MB volume carries a FAT of roughly 32 sectors, and each of the `fc` copies stores the Hamming-encoded table (`2 * fat_size` sectors). The [Filesystem builder](../builder.md) computes all of this for any combination.
