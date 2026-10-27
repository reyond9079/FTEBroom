# Alpha 84: intermediate interior-map optimization

Update: [alpha 85](performance-alpha85.md) diagnoses the 512-unit schedule
rebuild stalls and supersedes these performance results.

This is an intermediate release. The target of 60 FPS while flying through the
large map, including its panoramic view, has **not** been reached.

## Changes

- Pack only active indices into retained GPU buffers; reuse mutable storage and
  avoid uploading an unchanged index list.
- Parallelize conservative occluder rasterization and spatial visibility on large
  maps. Every worker owns its output; the main thread joins all workers before
  using the result. The serial fallback remains available.
- Use exact rectangle-depth queries and scanline coverage bounds instead of
  repeated per-cell edge tests. Preserve the conservative visibility proof.
- Retain a legal material schedule and filter it without allocating and sorting
  temporary run lists on each camera frame. Preserve depth-tie dependencies,
  custom-material barriers, transparent order and edited-layer interleaving.
- Prune schedule-only back faces only when they cannot face any camera within the
  cached 512-unit region. Rebuild the region before that guarantee expires.
- Keep masked Quake 1 textures transparent in regular, selected and drag previews.
- Render translated model instances during entity movement without mutating the
  original entity. Flush pending 2D line geometry before Model editor surfaces.
- Add a direct View > Brush outlines switch, using the existing saved setting.
- Size custom buttons from the translated label. Recompute translated clipping
  and ellipsis metrics when the caller supplied an English text size.
- Make native Wayland frame pacing nonblocking, including hidden windows and
  VSync changes, and avoid duplicate EGL/Wayland swap throttling.

No geometry simplification, texture downgrade or geometry draw-distance limit is
introduced. Small maps retain the established rendering path.

## Windows measurements

Core Ultra 7 155H / RTX 4060 Laptop GPU. Real WAD resources were loaded for the
30,327-brush E1M1 MAP (189,772 faces, 26,288 entity/container records). The original
MAP and WADs were not modified. MAP SHA-256:
`44c5dce383187921da2b47e9b8ce780334008f0597b9577c293846161625ae0e`.

The material browser, models, brush outlines, grid and entity/container bounds
were enabled; 8x MSAA and 16x anisotropy were retained. Each timed sample ran for
8 seconds, without screenshots, a stack sampler, concurrent builds or games.

- **1920 x 1080 window**, actual 3D viewport 1435 x 803: room 127.6 FPS stationary
  and **55.2 FPS moving** (p95 22.1 ms); panorama 72.7 stationary and **30.6 moving**
  (p95 36.7 ms).
- **2560 x 1440 window**, actual 3D viewport 2080 x 1163: room 118.7 FPS stationary
  and **57.2 FPS moving** (p95 21.0 ms); panorama 81.9 stationary and **32.3 moving**
  (p95 34.2 ms).

These are separate runs, not evidence that a larger resolution improves speed.
An earlier interior baseline was approximately 38 FPS in the room and 22 FPS in
this panorama. Exterior-box results in the alpha 83 report are explicitly marked
as exterior-only and do not demonstrate interior performance.

The room starts at `-1472 -160 296`, angles `0 90 0`; the panoramic camera starts
at `-3500 -3500 512`, angles `30 45 0`, inside the enclosing box and facing the
map's detailed geometry. FOV is 90 degrees. Moving samples translate along world
+X at 20 units/second. Start/end camera positions and viewport sizes were logged.

## Clean FTE control

The official FTE Windows build based on `f937b9d88` was downloaded and run in an
isolated folder without the ReyondBroom UI or csaddon. This is the same engine
base revision used by the editor.

The original MAP gave approximately 78 FPS in the room and 158 FPS in the
panorama, but its brush entities were not all rendered by that plain map-edit
configuration. A second, disposable control copy placed all 30,327 unchanged
brush blocks into one world entity and removed point-entity logic. It gave
approximately 617 and 712 stationary FPS at the matched 1435 x 803 viewport.

The second control changes rendering organization and removes editor work; it is
not an apples-to-apples editor benchmark. It demonstrates headroom in the engine
and directs further work toward editor preparation and material submission.

## Verification and remaining checks

Production-code tests cover retained indices, source revisions, material changes,
custom/alpha-tested ordering, edited-layer insertion, camera-region invalidation,
100,000 depth-rectangle queries, 4,608,000 scanline/cell comparisons and 47,068
physical occlusion queries. Eight raster partitions match the serial geometry
and label depth buffers bit-for-bit in 24 overlapping test scenes.

Reference/optimized images were compared inside the room, panorama and hall. The
hall was pixel-identical; room/panorama differences affected fewer than 0.01% of
viewport pixels at small color-rounding magnitudes. Models were disabled only
for that image comparison, not for the FPS measurements. Masked vines were also
inspected selected and unselected. Startup mouse creation, undo/redo, model-drag
instance invariance, translated buttons in seven languages at three UI scales,
and renderer restart were checked.

Both native and SDL2 Linux builds ran under an isolated Arch Linux / Plasma 6.8.1
Wayland test session, including VSync changes, renderer restart and normal exit.
This software-rendered virtual compositor does not certify physical-device mouse
capture, external drag/drop or GPU performance on a real Arch desktop.

User checks: open a large MAP with its WADs, fly inside it, create/move a brush,
move vertices, undo/redo, and toggle Brush outlines. Check masked vines and moving
model entities. Change to Russian and resize/dock Tools at 100%, 125% and 150% UI
scale. On Linux, additionally check cursor transitions, clipboard, external WAD
drag/drop and native/SDL2 VSync on the actual desktop.
