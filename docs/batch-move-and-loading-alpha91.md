# Alpha 91: faster map opening and group moves

## Changes

Translation now has a compact batch transaction for existing point entities and
texture-locked convex brushes. Point moves retain parsed properties, resolved
models and definitions. Native brush moves retain IDs, face topology and UVs;
they update geometry and collision bounds in place instead of deleting and
recreating each brush. Undo/Redo restore absolute captured values, avoiding
accumulated fractional movement error.

The document updates affected leaf geometry and retained parent bounds without
rebuilding the whole map. Point rendering keeps its buffers across translation.
Full point-cache builds use one owned vertex allocation instead of thousands of
individual allocations. Selection colors, model instances, geometry and depth
behavior are preserved. Definition lookup is indexed by class name, preserving
the original first-match behavior. Loading reserves brush storage geometrically
and avoids repeated full ID scans before the ID counter wraps.

Topology edits, patches, edits inside an opened group, brush moves with Auto
Integer Snap, and unlocked brush textures retain the established transaction
path. Entity strings containing escape sequences also retain that path. The new
native packet validates all brushes before mutation and validates byte ranges
without the legacy brush API's 65,536-element limit. An incompatible history
snapshot cancels replay with a message rather than aborting the viewport.
Fallback updates are explicitly sequenced and reject invalid history ranges,
including resource reload before the first edit.

## Measurements

Windows, Core Ultra 7 155H / RTX 4060 Laptop, 1920x1080 application window,
owned E1M1 copy with real WAD resources: 30,327 brushes, 189,772 faces,
56,616 document nodes, including the 22,880-entity misc_drip reproduction.
Original maps, WADs and mod files were not modified.

- Alpha 90 end-to-end open interval: 15.06 seconds. Alpha 91: 9.19 and
  9.12 seconds, approximately 39% less waiting. The interval ends after the first
  complete viewport frame, not merely after file reading. Both measurements
  include the same scheduled 200 ms interval before the file-open command.
  This excludes editor startup and initial game-resource configuration.
- Initial point render-cache preparation: 4,249 ms in the alpha 90 comparison;
  46 ms after pooling in alpha 91. Other loading stages still take time.
- Moving all 22,880 entities, including capture and document refresh:
  2,585 ms in alpha 90 versus 376 ms in alpha 91 (about 6.9 times faster).
  The separately captured mouse-release path measured 361 ms.
- Deferred point-render refresh after moving the group: 18–19 ms versus about
  1,054 ms in the alpha 90 comparison. Remaining selection/link, visibility and
  occlusion preparation can still add several hundred milliseconds for this
  unusually large selection. The 376 ms figure is not an input-to-present total.
- Moving 1,024 existing world brushes and refreshing the document: 76 ms.
  Geometry epochs and native IDs remain unchanged; exact bounds, render-face
  planes, Undo and Redo pass. No equivalent alpha 90 brush timing was taken.
- A stationary interior view averaged 116.20 FPS over eight seconds (median
  8.1 ms, p95 11.5 ms, p99 12.7 ms, maximum 18.29 ms). This is a specific
  interior view, not a guarantee for every camera position or the full-map view.

A separate slow strafe within the same interior averaged 110.99 FPS over eight
seconds (median 8.7 ms, p95 12.0 ms, p99 13.4 ms, maximum 15.53 ms).
The camera moved about 64 units; start/end captures confirm visible map geometry.
This short interior path does not establish performance across the entire map.

## Validation

- Production move/duplicate/cancel paths; world-limit rejection and Alt drag.
- Native batch path, stable IDs, fractional bounds and texture-lock UV biases.
- Sixteen Undo/Redo cycles without drift; geometry edit/remap followed by Undo.
- Invalid snapshot preflight leaves the history cursor and map unchanged.
- All 22,880 point origins and selection IDs checked on Move, Undo and Redo.
- Large brush bounds and cached face planes checked against current geometry.
- Packed point colors/vertices compare exactly to the original scripted path.
- Player model survives resource-epoch reload after packed buffers are allocated.
- Native Linux and SDL2 run the same small-scene checks in virtual Arch Linux /
  KDE Plasma 6.8.1 / KWin Wayland using software rendering. This checks Linux
  behavior, not physical Linux GPU performance.
- The engine patch reconstructs the tested engine sources. Packaging requires
  the new builtin and verifies corresponding sources, payload hashes and LF shell
  launchers.

## What to check

1. Open E1M1 with its actual WADs; compare textures, models, grid and outlines.
2. Move the large misc_drip group several times; Undo/Redo and check that every
   member stays visible, selectable and in the expected position.
3. Move a large brush selection with texture lock enabled, including fractional
   offsets; verify UV alignment, selection outlines and Undo/Redo.
4. Try a small map, duplicate, vertex editing, patches, opened groups, unlocked
   textures and Auto Integer Snap. These keep their existing behavior.
5. Edit entity properties or reload resources after moving; check player/model
   markers, bounds, connection lines and selection colors.
6. Save a copy, reopen it, and check the moved objects and textures again.
