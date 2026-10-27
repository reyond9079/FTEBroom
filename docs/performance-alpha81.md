# Alpha 81: retained brush commits on large maps

## What changed

Large documents retain their unchanged face/grid buffers and update an independently owned layer for edited worldspawn brushes. Create, move, vertex edits, deletion and Undo/Redo keep document node references stable. Deleted/replaced base faces stop rendering and occluding immediately; exact label visibility checks include the edited layer. Cached and uncached native passes merge edited surfaces in document order.

Material usage is updated from the transaction records rather than recounting every face on selection changes. The full recount now uses integer increments correctly. Point models, entity properties and links keep their caches across unrelated world-brush edits. Selected overlays use the explicit selection and cached locked nodes, with native ordered compaction. Ray picking and selection guide rays use conservative bounds before the existing exact intersection test. Selecting worldspawn brushes no longer rebuilds selection statistics for unrelated linked groups.

Unlit offline MAPs also avoid marking every material for a redundant lightmap/batch rebuild when a brush changes. Real relighting/lightmap allocations retain the original global path.

## Measurement conditions

- Windows, Intel Core Ultra 7 155H and NVIDIA GeForce RTX 4060 Laptop GPU.
- E1M1 FTE (2).map: 30,327 brushes, 26,288 entity/container records and a 32.3 MB source MAP.
- Actual authoring resources: 21 explicit WADs plus 14 mod WADs. Missing source materials remain missing; they were not replaced to improve the benchmark.
- Whole-map outside viewpoint, Material panel open, grid/outlines/names/bounds enabled, 8x MSAA and 16x anisotropy. No draw-distance limit, reduced texture resolution, geometry simplification or perspective viewport image cache.
- Eight-second samples. Mouse gestures exercise the production creation, move and vertex handlers. Resolution describes the editor client window, including its panels.

## Results

Independent FPS samples (idle / creation drag / move drag / vertex drag / create-and-confirm):

- 1920x1080: 77.45 / 71.12 / 74.71 / 88.70 / 82.82 FPS.
- 2560x1440: 73.98 / 79.72 / 81.21 / 88.35 / 88.11 FPS.
- Final create-and-confirm samples: p95 16.4 ms / 15.3 ms; longest complete frame 45.85 ms / 42.18 ms for 1080p / 1440p. Alpha 80's confirmation frame on this workload took about 2.72–2.89 seconds.
- The mouse-up handler itself measured 0–1 ms; document/cache preparation runs separately in the following frame and is included in the complete-frame figures. First-use index allocation can make a direct handler-plus-document check take several tens of milliseconds.
- Separate MAP saves: 4.98 seconds at 1080p and 4.54 seconds at 1440p. Saving is not asynchronous.

Vertex and confirmation samples were repeated after the final cache fixes. Idle/creation/move figures are separate eight-second samples, not one uninterrupted run.

FPS varies with hardware, viewpoint and background load; these figures are measurements, not a locked frame-rate guarantee. Linux native and SDL2 builds are checked separately in WSL/Xvfb with software rendering, so their frame rates are not comparable to the Windows GPU measurements.

## Correctness checks

- Actual large-map Create, Move, vertex commit, Undo/Redo and Delete checks passed.
- Sixteen create/Undo/Redo cycles and batch deletion kept the base geometry epoch unchanged.
- Material usage and ordered overlay candidates matched full scans before and after rebuilding.
- Canonical retained/full document trees matched; the 3D viewport after textured brush edits matched the full rebuild pixel for pixel. An unrelated blinking toolbar control is outside the comparison rectangle.
- Forty-eight selection-guide ray comparisons matched the exact reference, including 48 contacts.
- The representative small-map viewport remained pixel-identical to alpha 80.
- MSAA renderer restart restored ImGui and the edited document with 30,328 brushes.
- Linux native and SDL2 passed small-map selection/group/link checks and large-map Create/Undo/Redo/Delete with the same actual WAD collection under software rendering.
- Production C helper tests passed retained-buffer lifetime, source mutations, transparent order, sliced-range culling, conservative picking, ordered mark compaction, exact UV/grid formulas and live occluder retirement.
- The original MAP SHA-256 remained unchanged.

The original MAP and WADs are read-only inputs. Save checks write separate test copies.

## Scope and remaining work

The retained transaction path currently covers ordinary worldspawn brush geometry in large documents. Submodel/entity/group changes, patches, global resource/style changes, renderer recovery and large batches keep the established full rebuild path. Edited geometry is bounded; exceeding 2,048 retained brush entries or 128 history records in one update safely rebases the snapshot. This avoids unbounded memory growth. Full MAP saving remains synchronous and still takes several seconds on this workload.

The next optimization step is retained updates for brush submodels and ancestor bounds, followed by immutable/background save snapshots. See [the broader optimization plan](performance-plan.md).

## Reproducing validation

Build the matching engine patches, QC and UI plugin. Use an isolated runtime with the same resource collection, outside camera and quality settings. Enable `ftebroom_skip_setup` only in that test runtime. The gated `ftebroom_fullbench_qa` modes 6/7/8 exercise large-map Create/Undo/Redo/Delete, live gesture measurements, and retained commit/material/selection/tree comparisons respectively. `ftebroom_profile_commit 1` prints slow preparation stages; it is disabled normally.

The production native helpers are checked by `test-bulk-attributes.py`, `test-native-faces.py`, `test-geometry-occlusion.py` and `test-conservative-occlusion.py` under `tests/`. `check-incremental-edits.py` compares the gated stress log and canonical fast/full document snapshots, and optionally checks the images byte for byte.
