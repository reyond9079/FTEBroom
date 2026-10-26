# Alpha 77: large-map rendering

This update targets large editable MAP documents without reducing texture quality, antialiasing, geometry detail or draw distance.

## Changes

- Retain brush vertices, surface grids and point-entity boxes in bounded GPU buffers. Rebuild them after edits, filtering, selection changes or a graphics-device restart. Unsupported renderers and exhausted cache budgets retain the previous submission path.
- Resolve shared editor materials once per material, rather than once per face. Combine equivalent render states while preserving primitive order, UVs, colors and transparency.
- On large Quake 1 maps, conservatively reject objects whose complete bounds are behind opaque surfaces. Only fully covered screen cells can prove occlusion; local perspective-depth bounds, expanded object bounds and numerical margins keep uncertain objects visible. Sky, fences, liquids, translucent surfaces and helper materials do not become opaque geometry occluders.
- Keep the exact entity-label visibility index, with a small associative occluder cache. Reuse visibility results only when the label position, camera and indexed geometry are unchanged.
- Reserve the complete renderer batch pool before drawing a large scene. Previously its slow growth could temporarily omit geometry, making early screenshots and FPS readings misleading.
- Fix an undersized 64-bit entity-resend allocation, incomplete copying of pending records and boundary checks. This could corrupt memory when opening a document with many entities.
- Restore editor shader definitions after restarting the renderer. Remap restored brush references in one pass, preserving selection and Undo/Redo without repeatedly scanning the whole document.

The new geometry-occlusion path is limited to perspective Quake 1 editing with at least 30,000 cached brush faces. It is disabled during geometry previews and clipping. Small maps retain their existing rendering path. Picking, collision queries, saved MAP geometry and compiler output are unaffected.

## Measurement conditions

The supplied `E1M1 FTE.map` contains 30,327 brushes and 26,288 entities. Tests view the complete map from outside its edge, then move the camera while keeping the map in view. Measurements use Windows OpenGL on an NVIDIA RTX 4060 Laptop GPU and Intel Core Ultra 7 155H, with the existing grid, outlines, bounds, labels and filtering enabled.

Measured eight-second averages with the complete MAP in view:

- 1920×1080: **66.3 FPS** stationary and **64.7 FPS** while moving.
- 2560×1440: **67.1 FPS** stationary and **68.1 FPS** while moving.

The original 22 external WADs were not supplied. These measurements therefore use fallback materials, and do not establish the performance or appearance of the original complete texture set. A measured average above 60 FPS on this machine is not a minimum-framerate guarantee for other hardware or maps. Linux software-rendering runs check correctness, not hardware FPS.

The user's source MAP remains unchanged: SHA-256 `4d40ec4a51adfcd956e776f86bdf0c9dec2ef9f28aab328029af497b5add8626`.

## Validation

- Six large-map viewport comparisons, including outside, below, side and interior cameras, match the unculled reference pixel for pixel.
- Small-map visual checks cover selection, face hover, multiple selection, patches, point entities, perspective and 2D views.
- UV checks verify unsaved geometry, selection and Undo/Redo across an MSAA renderer restart on Windows and Linux.
- Native tests cover retained-buffer lifetime, odd-vertex CPU-array alignment, material changes, index bounds, complete scene batches, conservative visibility, VM arguments and entity-resend allocation guards.
- Engine patches are checked against their preceding source versions so the published source archives reproduce the modified files.

## What to check

1. Open your large map, fly through rooms and inspect the complete map from outside. Check FPS at your usual resolution with your actual WADs loaded.
2. Inspect windows, thin walls, sky, transparent brushes and point entities. Objects should become visible immediately when exposed.
3. Move, resize, clip, delete and undo brushes; change material filters and group visibility. Check that surfaces, outlines, grids and labels update immediately.
4. Apply MSAA and verify the map, materials, selection, Undo and Redo afterward.
5. Compare a small map with the previous release, including 2D views and patches. Its appearance should stay the same.
6. On Linux, check the native and SDL2 packages separately, including startup, external resources and ordinary editing.

Diagnostic switches are `ftebroom_geometry_occlusion 0` for geometry visibility, `ftebroom_cached_meshes 0` for retained GPU buffers and `ftebroom_merge_missing_materials 0` for equivalent-material batching. These are optional comparisons, not setup requirements.
