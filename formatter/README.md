# NiFAT32 Formatter

The formatter creates a raw NiFAT32 filesystem image. The image can be used
in tests, opened through the NiFAT32 library, or written directly to an SD
card or another block device.

> **Important:** NiFAT32 is different from standard FAT32. Regular FAT32
> drivers in Linux, Windows, and macOS may not recognize the image.

## Building

GCC, Make, and a POSIX-compatible system are required.

```bash
cd formatter
make
```

The `formatter` executable will be created in the current directory.

To remove the compiled executable:

```bash
make clean
```

## Quick Start

Create an empty 64 MB image using the default settings:

```bash
./formatter -o ../nifat32.img
```

Create a 128 MB image and copy the contents of `../data` into it:

```bash
./formatter -o ../nifat32.img -s ../data --volume-size 128
```

The directory passed with `-s` is processed recursively. Its files and
subdirectories are added to the root directory of the new image.

## Options

| Option | Description | Default |
| --- | --- | --- |
| `-o PATH` | Output image path | Required |
| `-s PATH` | Source directory to copy into the image | Empty image |
| `--volume-size N` | Image size in megabytes | `64` |
| `--spc N` | Sectors per cluster | `8` |
| `--fc N` | Number of FAT copies | `4` |
| `--bsbc N` | Number of boot sector copies | `5` |
| `--b-bsbc N` | Number of deliberately corrupted boot sector copies used for testing | `0` |
| `--jc N` | Number of journal sectors | `2` |

Full example:

```bash
./formatter \
  -o ../nifat32.img \
  -s ../data \
  --volume-size 128 \
  --spc 8 \
  --fc 4 \
  --bsbc 5 \
  --b-bsbc 0 \
  --jc 2
```

These filesystem geometry values must match the values passed to the
NiFAT32 library when the image is opened.

## Using an SD Card Image

The formatter places the filesystem directly at sector zero. It does not
create an MBR or GPT partition table.

Create an image file of the required size:

```bash
./formatter -o ../sdcard.img --volume-size 1024
```

On Linux, the resulting image can be written to an SD card with:

```bash
sudo dd if=../sdcard.img of=/dev/sdX bs=4M conv=fsync status=progress
```

Replace `/dev/sdX` with the actual SD card device and verify the device path
with `lsblk` before running the command. `dd` will destroy all existing
partitions and data on the selected device.

Do not pass `/dev/sdX` directly to the formatter through `-o`. The formatter
opens its output with truncation enabled and calls `ftruncate()`, so it is
designed to create a regular image file.

## Limitations and Safety

- An existing file passed through `-o` is completely overwritten.
- The image size is specified as a whole number of megabytes.
- The image must not be larger than the target device.
- The path passed through `-s` must be an existing directory, not a file.
- File names are converted to the 8.3 format.
- `--b-bsbc` is intended only for corruption recovery tests.

## Verifying the Image

Check the size of the resulting image:

```bash
ls -lh ../nifat32.img
```

To open the image with the test Unix utility, run the following commands from
the project root:

```bash
gcc unix_nifat32.c nifat32.c src/*.c std/*.c -Iinclude -o unix_nifat32
./unix_nifat32 nifat32.img 64 512 5 2
```

The arguments `64`, `512`, `5`, and `2` specify the image size in megabytes,
sector size, number of boot sector copies, and number of journal sectors.
They must match the settings used to create the image.
