# Filesystem builder

Place NiFAT32 on your flash, inspect the separation between redundant copies, and move a localized particle beam across the memory map.

<a class="text-link tool-link" href="pages/builder.html" target="_blank" rel="noopener noreferrer">Open builder in a full window</a>

<iframe class="embed" src="pages/builder.html" title="NiFAT32 square flash map, particle beam, and filesystem builder"></iframe>

## Square flash map

The square chip view highlights every boot, FAT, journal, and error-storage copy at the flash offsets calculated by the formatter. Choose a copy to highlight all of its mapped tiles; hover over a tile to inspect its address range. The current volume offset is included, so moving the volume moves its copies on the chip.

STM32 and ATmega presets show **internal program flash**. ESP32 presets show the **external flash memory**, rather than placing storage inside the ESP32 processor. The preset selector groups these three families first.

The default view maps ascending addresses to rows, with an optional serpentine order. These are **spatial models**, not measured physical die layouts. Known erase sizes define the tile subdivision, but an address range alone does not establish physical distance on silicon. Square package outlines are schematic. On STM32 presets, outlines group tiles into the unequal erase sectors; ATmega tiles aggregate two 256-byte pages into one 512-byte NiFAT32 sector.

For device-specific coordinates, open **Use measured address-to-position coordinates** and import a JSON map. The map must identify the chip, flash size, coordinate units, source, array dimensions, and a physical rectangle for each address range. Export the current model for a complete editable example. Imported coordinates are identified as user-supplied; exported grid models retain their model label.

## Move the particle beam

- Drag the **circular beam spot** across the memory array, or position it with the X/Y sliders. Arrow keys move the beam; Shift moves it faster.
- Change the radius, or switch to a **particle track** and set its angle. Radius and track half-width are percentages of array width; imported maps in `um` or `mm` also show the corresponding coordinate distance.
- Inspect the intersected-copy count separately for FAT, boot, journal, and error storage. **Center on a FAT copy** gives an immediate example; **Hide beam** clears the overlay.
- Choose a structure family to isolate its copies, or select an individual copy below the map to reveal its entire footprint.

An intersection exposes the **whole mapped address range**. It does not claim that every cell flips or that a copy becomes unreadable. FAT and journal voting is per entry: touching multiple copies need not damage the same entry in each. An untouched majority is a conservative indication of remaining redundancy; its absence is not proof of data loss.

## Volume and device configuration

Configure flash offset, volume size, sectors per cluster, and the numbers of FAT copies, boot copies, journals, and error-storage copies. The linear address map remains available under **Linear address map & partitions**; the sector map supports zoom and pan.

Validation checks volume bounds, occupied partition overlap, metadata collisions, and allocator RAM. Preset partitions are examples, not a partition table read from your board. The builder estimates ROM/RAM from host measurements; see [reference footprint](docs/footprint.md) for the build configurations and [creating an image](docs/image.md) for formatter commands.

## Device references

- [STMicroelectronics RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f4xx-reference-manual-stmicroelectronics.pdf): STM32F407 erase-sector organization.
- [Espressif partition tables](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/partition-tables.html): flash-relative partition addresses.
- [Microchip ATmega640/1280/1281/2560/2561 datasheet](https://ww1.microchip.com/downloads/en/devicedoc/atmega640-1280-1281-2560-2561-datasheet-ds40002211a.pdf), section 30.5: ATmega2560 flash pages are 128 words (256 bytes); EEPROM is a separate 4 KiB memory.
- [Winbond W25Q32JV datasheet](https://www.winbond.com/resource-files/W25Q32JV%20RevJ%2012242024%20Plus.pdf): example external flash organization.

These references support address and erase geometry. No measured address-to-die coordinate map is bundled with the presets.
