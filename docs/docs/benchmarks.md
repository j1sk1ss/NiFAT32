# Benchmarks & comparisons

**NiFAT32 configuration:** five boot copies, five FAT copies, five metadata backups; caches and indexing disabled.

## Throughput

NiFAT32 throughput divided by the comparison filesystem's throughput. **Above 1× is faster.**

<!-- performance:plots -->

<details>
<summary>Exact throughput ratios</summary>

<!-- performance:table -->

</details>

## Energy per operation

Chart: NiFAT32 energy / comparison filesystem energy. **Below 1× uses less energy.**

<!-- energy:chart -->

<details>
<summary>Exact energy ratios: comparison filesystem / NiFAT32</summary>

<!-- performance:energy -->

</details>

## Fault tolerance

<!-- research:chart -->

LittleFS uses the `ramrsbd` Reed–Solomon block layer in the fault-injection test.

## Advantages & tradeoffs

| Compared with | NiFAT32 advantage | Tradeoff |
| --- | --- | --- |
| FAT32 | Fewer failed runs under bit-flip injection; metadata correction and repair | FAT32 is 3.50–16.2× faster in the measured operations and uses less energy. |
| LittleFS | 3.26–10.43× the measured throughput; lower gross energy per operation | LittleFS + RS has slightly fewer failures in the fault-injection test. |
| YAFFS2 | 2.16× write throughput; about 48% of the gross write energy | YAFFS2 is faster and uses less energy for the other five operations. |

Results apply to these benchmark configurations. File payloads need separate protection. See [footprint & build flags](footprint.md) for memory costs.
