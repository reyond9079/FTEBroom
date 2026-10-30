# Alpha 108: automatic generated-cache cleanup

ReyondBroom now maintains its generated disk caches at startup and shutdown. No cleanup scans run during interactive editing. A shared lifetime lock protects other running editor instances; cleanup requires exclusive ownership. The initial cleanup of a heavily accumulated installation may take longer than a normal launch.

## Retention rules

- Currently indexed Quake 1/II texture images are pinned by shader manifests and never evicted for age or budget.
- Unreferenced generated TGA images expire after 24 hours. When a cache exceeds 512 MiB, the oldest unreferenced images are also removed toward 256 MiB. Referenced images may exceed that budget.
- Only recognized generated texture names are eligible; arbitrary images in those folders remain untouched. Missing/unreadable shader manifests disable texture eviction.
- Reserved `_ftebroom_open_<number>_<number>.map` engine-input copies are removed when no other editor instance is using the installation. They are neither original maps nor autosaves. The current/pending source paths are protected.
- The two newest mod-preview sessions are retained. In older sessions, derived files expire after seven days; MAP, BSP and WAD copies remain protected.
- User maps, WAD archives, autosaves, mod downloads, compiler working directories in mods and BSP export projects are not swept.
- Filesystem links are not followed. A bounded `ftebroom/cache-cleanup.log` records removals and reclaimed bytes.

This is generated-cache maintenance, not a general disk-cleaning operation. It never recursively deletes the installation, a game directory, or an arbitrary user-selected folder.

## Checks

1. Open a map, edit it, save, close the editor and reopen it. Verify textures and autosaves remain available.
2. Compile and preview a map after cleanup; evicted unused textures regenerate when their resources are indexed again.
3. Open a second editor instance: cleanup must not remove live staging copies while either instance is still using them.
4. Read `ftebroom/cache-cleanup.log` for reclaimed file/byte counts.

Tests cover pinned images, budget pressure, reserved temporary names, protected source paths, WAD/BSP/autosaves, missing manifests, filesystem links and preview-source retention on Windows and Linux.
