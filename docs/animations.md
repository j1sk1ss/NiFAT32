# How it works

Follow the bytes through NiFAT32. Step through each operation, play it end to end, or inject a fault and watch the protection layers respond.

<a class="text-link tool-link" href="pages/animations.html" target="_blank" rel="noopener noreferrer">Open walkthroughs in a full window</a>

<iframe class="embed" src="pages/animations.html" title="Interactive NiFAT32 layout, mount, write, and upset recovery walkthroughs"></iframe>

## Explore the four scenes

- **Layout** — see where boot copies, FAT copies, journals, and error areas land, and where the data clusters live.
- **Mount** — follow boot sector fallback, repair, geometry readout, and journal replay.
- **Write** — trace allocation, write-ahead logging, directory updates, and FAT replication.
- **Upset & repair** — try real Hamming(15,11) decoding and majority voting. A single flipped bit is corrected during decoding; a double-flipped copy can lose the vote. Corrupting a majority can defeat recovery.

Use **Previous**, **Play / Pause**, and **Next** to control each scene. In the upset scene, inject a fault first, then step through read, decode, vote, and repair.

Read the [architecture guide](docs/architecture.md) for the algorithms behind the visualization.
