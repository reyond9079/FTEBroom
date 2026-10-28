# Alpha 93: selection rendering, centered BSP models and audio tools

## Performance changes

Static selected/locked brush and patch overlays are retained in shared-vertex chunks. Flying the camera no longer reconstructs every face and contour through QC. Changes to selection, face masks, visibility, geometry, colors or view mode invalidate the cache; active modeling previews retain their replacement-aware path. Fill, line opacity and depth behavior are preserved. A separate engine buffer-flush fix restores contours that previously disappeared after the first large overlay batch; large selections therefore show their complete highlighting. Small selections continue to use the existing path.

Material inspector values are refreshed after document, selection or default-material changes rather than rescanning thousands of faces on every frame. Inactive hover highlighting no longer invalidates selection meshes as the pointer crosses different brushes.

Opaque masked WAD materials use the existing GPU material alpha-test implementation again. Previously one such material could force the whole retained map onto the slower fallback. Transparent/faded masks retain their dedicated shader and cutout-before-opacity behavior.

On the Windows test machine (RTX 4060 Laptop GPU), the same interior camera flight through the large E1M1 copy with real WADs, 30,327 brushes and 189,772 faces measured:

- 1920×1080, no selection: **21.5 FPS in alpha 92 → 104.3 FPS**.
- 2560×1440, no selection: **90.7 FPS** in the release build.
- 2560×1440, 8,192 selected brushes: **41.4 FPS**.
- Same 1920×1080 route with 8,192 selected brushes: **5.7 FPS → 44.8 FPS**.
- Selected-flight median frame time: **168.8 ms → 20.9 ms**; 95th percentile: **237.3 ms → 25.5 ms**.

A fixed-camera image comparison against the corrected reference path differed in only 4 of 2,073,600 pixels.

These are measured averages on one machine, not a guarantee of 60 FPS with every selection. Selection changes still have a one-time mesh preparation cost; a 219 ms worst frame occurred in the selected measurement. No draw-distance, map detail, texture resolution or small-map visual defaults were reduced.

The large-map regression run exercised BSP entity placement and Undo/Redo, 512-brush deletion and restoration, fractional batch movement/texture lock, creation, movement and vertex-edit confirmation. All retained-cache assertions passed. Command/document timings were about 80 ms for Delete, 36–45 ms for Undo/Redo and 9–59 ms for the tested single-brush confirmations; the following render frame is not included.

## Centered BSP export

Use **File → Export selected brushes as BSP model...**, or the matching viewport context-menu action. Only the selected brushes are exported. Their combined bounding-box center becomes **0,0,0** in the compiled model. Visible geometry dimensions, collision and surface texture placement are preserved (Q3 compiler bounds can include their usual padding); the source selection is not moved. This applies to Q1 BSP29/BSP2 and Q3 BSP export, with or without an atlas.

Point-model and classname edits now update entity state without rebuilding unrelated map geometry. Brush-owned internal `*` models retain the guarded geometry refresh path. Dropping a BSP still needs to load that model once, but no longer discards the whole world cache just because its `model` property changed.

## BSP UV over the texture

Double-click a BSP in the file manager to open **Model editor**, then press **BSP UV / texture...** in the inspector or choose the same item under **View**.

- Choose a material from the list. Its texture is displayed with yellow UV lines over it.
- Toggle **UV overlay**, use **Fit**, the zoom control or the mouse wheel; use the canvas scrollbars to pan.
- Embedded Q1 textures use the loaded game palette. Q3 materials resolve their editor image from the active resource catalogue.
- Tiled UVs wrap onto the texture tile. On an atlas-exported model, the atlas and its packed UV boundaries are shown directly.
- Q3 triangle surfaces show triangle edges; curved patches show the quadratic UV control network. Missing images leave a neutral canvas with the UVs.

Parsing and overlay rasterization run in background jobs, with bounded input/complexity checks. The overlay is cached as an image instead of emitting thousands of interface lines every frame. This is read-only inspection; it does not modify the BSP or paint the texture. The separate atlas export still creates `atlasN.tga`, `uvN.tga` and `PAINTING.txt` for painting in an image editor.

## Sound editor tools

Double-click a sound in the file manager. **Seek / playback** is selected initially: click or drag on the waveform to continue playback from that time without changing the trim range. **Play/Pause** resumes or pauses; **Stop** returns to the beginning of the active playback range.

Choose **Trim** to select the start/end range on the waveform or edit its numeric fields. **Apply trim when saving** controls whether that range is written. Returning to Seek previews the complete source, even when a trim range has been prepared. Volume and speed remain available. **Save with backup** replaces the original file and retains its backup, as before.

## Check after updating

1. Open a large map with its real WADs. Fly inside it, select a large brush group and repeat the route. Change selection, lock/hide objects and switch 3D/2D views; overlays must refresh correctly.
2. Create, move, edit a vertex, duplicate and delete brushes. Check Undo/Redo and material/UV values, including a large selection.
3. Verify masked foliage/grates, translucent service materials and fading during a drag. Small-map appearance should match the previous build.
4. Export an off-center brush selection to BSP, with and without an atlas. Open it in Model editor: its bounding box should be centered at zero and retain its dimensions. Drop it into a large map and check placement and Undo/Redo.
5. Open BSP UV / texture. Check material switching, yellow boundaries over the image, overlay toggle, Fit, zoom, tiled textures and atlas textures.
6. In Sound editor click halfway through a sound: playback should start there. Switch to Trim, choose a range, switch back to Seek and verify full-source playback. On a disposable sound, verify volume/speed, saving, resulting duration and backup.

Builds are provided for Windows x64, native Linux x64 and Linux SDL2 x64. Linux GUI checks run in the Arch test environment with software rendering; Windows FPS results must not be interpreted as Linux hardware measurements.
