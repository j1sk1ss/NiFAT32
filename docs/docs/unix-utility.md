# Unix utility

The Unix utility mounts a NiFAT32 image file and exposes it through a small interactive shell. It is the same tool the integration test scenarios drive.

Build:

```bash
make unix
```

Run:

```bash
./builds/unix_nifat32 nifat32.img 64 512 5 2
```

Arguments are image path, image size in MB, sector size, boot sector copy count, and journal count.

Supported commands include `cd`, `ls`, `mkdir`, `mkfile`, `read`, `write`, `trunc`, `cp`, `mv`, `rm`, `frename`, `rs`, `ws`, and `get_le`.

| Command | Purpose |
| --- | --- |
| `cd <dir>` | Change the working directory (8.3 uppercase names) |
| `ls` | List the current directory |
| `mkdir <name>` | Create a directory |
| `mkfile <name>` | Create an empty file |
| `read <file>` | Print the file content |
| `write <file> <text>` | Store text in the file (first token only) |
| `trunc <file> <size>` | Truncate the file |
| `cp <src> <dst>` | Copy a file (shallow) |
| `mv <src> <dst>` | Move/rename a file |
| `rm <file>` | Delete a file |
| `frename <old> <new>` | Rename a file |
| `rs` | Read all boot sector copies (debug) |
| `ws` | Write all boot sector copies (debug) |
| `get_le` | Drain the persistent error ring (debug) |

The prompt is `<NAME> > ` without a trailing newline, and paths are 8.3 uppercase, so `write` stores only its first token and `read` prints until the first NUL byte. Those quirks are what the integration scenarios assert against.
