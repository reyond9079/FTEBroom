# Large-map editing, saving and material performance plan

Status: investigation and proposed work. Only the Quake 1 material correctness fixes are implemented with this investigation. The optimization phases below are not implemented yet.

## Reference inspected

Official TrenchBroom master commit `5cc553585401bcc867726e960189c5515397f1a8`:

- [BrushRenderer.cpp](https://github.com/TrenchBroom/TrenchBroom/blob/5cc553585401bcc867726e960189c5515397f1a8/lib/TbRenderLib/src/BrushRenderer.cpp): individual invalid brushes, per-brush VBO allocation references, material-grouped indices and separate opaque/transparent rendering.
- [BrushRendererArrays.cpp](https://github.com/TrenchBroom/TrenchBroom/blob/5cc553585401bcc867726e960189c5515397f1a8/lib/TbRenderLib/src/BrushRendererArrays.cpp): retained arrays, allocation tracking and dirty upload ranges.
- [Resource.h](https://github.com/TrenchBroom/TrenchBroom/blob/5cc553585401bcc867726e960189c5515397f1a8/lib/TbGlLib/include/gl/Resource.h) and ResourceManager.h: distinct unloaded/loading/loaded/ready states, background tasks and bounded processing.
- [MapFileSerializer.cpp](https://github.com/TrenchBroom/TrenchBroom/blob/5cc553585401bcc867726e960189c5515397f1a8/lib/TbMdlLib/src/MapFileSerializer.cpp): per-node strings prepared with tasks, followed by ordered serialization.

These are architectural observations, not a measured FPS comparison against a running TrenchBroom build. Qt is not the reason brush geometry is cheap to update.

## Confirmed ReyondBroom code paths

1. `fb_render_faces` and `fb_surfacegrid` disable their native retained passes while a gesture or geometry preview is active. Unchanged map geometry then returns to interpreted face/index submission. Geometry occlusion is also disabled during previews. This makes navigation FPS an inadequate editing benchmark.
2. Structural history changes trigger `fb_document_rebuild`; global geometry epochs invalidate multiple document-wide caches. Editing one brush can rescan and upload unrelated geometry.
3. The material browser caches thumbnail handles, but clears the entire thumbnail cache above 512 entries. It also rebuilds and sorts the filtered material vector every frame. Virtualized drawing limits visible tiles, but does not remove that preceding work.
4. Q1 asset indexing reads/decodes all selected WADs and emits generation-named TGA files for every texture. An unchanged resource reload can repeat work and leave old generation files behind.
5. Saving writes an engine intermediate MAP, reads it into C++, parses/formats face lines again for Q1 conversion, writes an adjacent temporary destination, and rereads/indexes the result. Atomic replacement is useful; repeated formatting and I/O need profiling. CPU formatting, filesystem writes and archive rewriting must be measured separately before choosing a fix.

## Phase 1: reproducible measurements

Use the same camera, quality, profile and actual resources on the user's E1M1 MAP at 1920x1080 and 2560x1440. Record CPU/GPU frame times, p50/p95/p99 and the longest pause for idle, camera flight, mouse brush creation, move, vertex drag, release, Undo/Redo and Save. Split document rebuild, visibility, surfaces, overlays, picking, material browser and save stages. Compare the Material panel visible/hidden and a minimal WAD selection/full selection, keeping displayed map textures equivalent.

The earlier fallback-material flight averages do not establish editing performance with the real WAD collection. The desired target is at least 60 FPS during ordinary gestures with unchanged visual quality; 16.7 ms is the corresponding frame budget, not a result already demonstrated.

## Phase 2: keep static geometry fast during a gesture

Keep the unchanged map in native retained geometry while drawing a small dynamic preview separately. Exclude only replaced brushes from the static pass. Include both original and preview bounds in visibility updates. Never use stale occluders from a moving/deleted wall. Verify transparent ordering, outlines, grids, sky, selection and immediate revealing of objects behind edited geometry. Do not substitute a cached viewport image or reduce draw distance.

## Phase 3: incremental edits and GPU updates

Publish changed model/brush IDs from transactions and Undo/Redo. Separate topology, positions, UV/material, selection and visibility revisions. Update affected surface/grid ranges, ancestors and spatial-index leaves. Use reusable bounded native buffer blocks and dirty ranges; avoid whole-map vertex uploads and VM copies after a single edit. Retain full rebuilds for loading, renderer recovery or genuinely global changes. Frame-budget rebuild work where it cannot be incremental, without displaying an editable stale document.

## Phase 4: materials and resource caching

Cache filtered/sorted material lists until their inputs change. Replace wholesale thumbnail clearing with a bounded LRU cache and a per-frame decode/upload budget. Keep scene textures independent from thumbnail eviction. Index WAD entries once, decode on demand and retain disk-converted images using archive identity, entry content, palette hash and converter version. Invalidate only changed resources and do not rescan directories every frame. Measure VRAM pressure before adding texture residency eviction; preserve current texture resolution, mip quality and filtering.

## Phase 5: saving

Start with buffered serialization and removal of redundant intermediate parsing/copies. Serialize directly into the requested format from an immutable document snapshot. Cache text for unchanged brushes/entities and prepare changed blocks in workers; write the ordered output to a temporary file and replace the destination atomically. Workers must never read mutable engine/QC objects. Associate completion with the captured revision so edits made during saving remain dirty. Preserve backup/error behavior, archive limits, plane precision, UV axes, entities and metadata. Autosave may use a separate incremental recovery journal later; the user's MAP remains a complete standard file.

## Acceptance and risks

Compare small-map images and large-map interior/exterior views before and after each phase. Exercise selecting, clipping, transparent surfaces, material replacement, Undo/Redo and MSAA restart. Round-trip geometry/UV/entity data through saved maps and compilers. Interrupt a test save and verify the original destination remains intact. Check memory after repeated edits/reloads and compare Windows, Linux native and SDL2. Publish separate measured results for navigation, active drag and save duration. No fixed speedup or save-time claim is justified before these measurements.
