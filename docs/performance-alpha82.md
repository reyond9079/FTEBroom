# Alpha 82: large-map rendering and vertex previews

## Changes

Large maps group independent opaque faces into fewer draws. A bounded dependency graph preserves document order for potentially depth-tied surfaces, including same-material pairs. Custom/transparent materials and edited-layer insertions remain ordering barriers. Material-major retained chunks avoid creating a GPU object for every material transition. Geometry-local bounds, adjacency and a conservative document order are retained across camera motion; a stationary view can refine its visible order and reuse its indices. This renders geometry every frame, without a viewport-image cache, mesh simplification or a geometry draw-distance limit.

The renderer no longer silently omits remaining scene batches when its temporary batch pool reaches capacity. Stable overflow blocks cover that frame; the normal pool grows on the next frame. This fixes a confirmed missing-geometry path that can affect large editing previews. Vertex previews replace and commit only brushes containing affected vertices, leaving other selected brushes in the retained scene. Invalid hulls keep the preceding valid preview.

Projected labels cache their positions and visibility only while camera/projection, viewport, source revision and occlusion geometry remain unchanged. They are still drawn each frame; camera movement invalidates this cache.

## Brush grid distance

As requested, the surface grid starts fading at 512 world units and disappears at 1,024. The default follows the distance range in the TrenchBroom face shader inspected during this work. The shader fades per fragment; native passes conservatively skip faces entirely beyond the limit. Line thickness, grid spacing, textures, brush edges and geometry are unaffected.

Change **Map → View options → Antialiasing and texture filtering → Grid fade start / Grid draw distance**. Settings persist. Draw distance `0` restores unlimited surface grids. The construction-plane grid is separate.

## Measurements

Windows, Core Ultra 7 155H, RTX 4060 Laptop GPU. Read-only E1M1 FTE (2).map: 30,327 brushes, 26,288 entity/container records, 189,772 faces. Actual authoring resources: 21 explicitly referenced plus 14 auto-discovered WADs; 6,028 distinct WAD names, 6,072 browser materials. Missing source materials remain missing. Material panel open, outlines/names/bounds enabled, 8x MSAA and 16x anisotropy, uncapped rendering.

Each result is an eight-second sample. Resolution is the editor client window, including panels. The heavy edge camera starts at `-8000 0 128`, angles `0 0 0`, FOV 90. This is different from the lighter whole-map framing in the alpha.81 report.

- **1920x1080 stationary:** reference 21.13 FPS; optimized with unlimited grids 63.26 FPS, about 3.0x. With the new default grid fade: 62.69 FPS, p95 18.8 ms, maximum frame 22.23 ms.
- **2560x1440 stationary:** reference 19.55 FPS; optimized with unlimited grids 61.87 FPS, about 3.2x. With grid fade: 61.05 FPS, p95 20.8 ms, maximum 24.91 ms.
- **Continuous camera flight:** 20.13 / 20.59 FPS at 1080p / 1440p. Corresponding p95 frame times: 56.0 / 56.2 ms. Actual camera displacement was checked. The 60 FPS navigation target is **not achieved**.
- **Actual vertex drag:** 59.24 / 66.83 FPS at 1080p / 1440p, p95 21.3 / 19.1 ms, maximum 31.52 / 25.79 ms. The vertex delta changed during the sample. Exactly one source brush was masked; the other scene nodes remained available.

The main stationary gain comes from batching and reuse, not the grid fade. Separate sample variation is larger than the grid fade's measured difference on this viewpoint. No locked minimum FPS is claimed. Linux functionality is checked under WSL/Xvfb software rendering; this does not establish comparable Linux GPU FPS.

**Benchmark correction:** earlier alpha.80 camera-flight numbers in the performance plan were not real flight: the UI reset the command's movement value each frame. They have been withdrawn as navigation evidence. The corrected gated test overrides movement inside the production camera step and prints actual start/end positions.

## Validation and scope

- Reference versus optimized images with unlimited grids matched at 1440p; at 1080p one pixel differed. Near textured surfaces visibly retain their grid with the distance limit enabled.
- A two-brush vertex regression checked that only the affected brush is replaced and that the untouched brush's topology and positions remain unchanged. A 32-sided cone apex moved, retained topology and undid correctly.
- Production helper tests cover 30,000-face retained draws, transparent barriers, overlapping-face dependencies, filtered retained ordering, buffer invalidation, renderer fallback and release; 100,000 renderer batches retain stable pointers with no omissions. Label tests exercise projection, source, occlusion, viewport, VM relocation, disable and legacy calls. Existing native face and conservative occlusion tests pass.
- The original MAP SHA-256 remains `44c5dce383187921da2b47e9b8ce780334008f0597b9577c293846161625ae0e`. Test edits were not written to it or its WADs.

This is an intermediate performance release. Continuous navigation still spends substantial CPU time rebuilding visibility/index lists and label visibility, and needs further work. The existing incremental-edit scope and synchronous-save limitations remain as described in [alpha.81](performance-alpha81.md). This release does not claim a new save-time improvement.

## What to check

1. Open the large MAP with its real WADs; compare stationary viewing, camera flight and vertex dragging separately.
2. Move a vertex on one of several selected brushes: untouched brushes must remain visible. Check invalid hulls, cancel, commit, Undo/Redo, and editing a cone apex.
3. Move toward/away from textured brushes: surface grids should remain crisp nearby and fade smoothly at 512–1,024 units. Change grid spacing and both distance fields; restart to check persistence. Distance `0` should restore unlimited grids.
4. Check a small map, transparent/coincident surfaces, brush entities, labels, outlines, selection and MSAA renderer restart.
5. On Linux native/SDL2, verify the same editing and grid controls on your GPU. WSL software rendering cannot substitute for that check.
