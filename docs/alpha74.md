# ReyondBroom alpha 74 — large MAP performance

## Changes

- Engine brush/patch queries use bounded per-model hash indexes instead of a
  full linear scan for every ID. Indices, not raw brush pointers, are cached.
  Insert, delete, swapped array entries and model destruction invalidate them.
  Missing IDs and malformed duplicate IDs retain their previous semantics.
- MAP source-line indexing builds entity/brush ordinal maps once. The former
  nested scan could hit the VM runaway limit on a 20,000-brush MAP.
- Surface grid/edge caches use four packed allocations instead of four heap
  allocations for every face. Entire off-screen brush ranges are rejected before
  individual face tests. There is no distance cutoff or detail reduction.
- Ordinary unselected opaque world brushes are rendered by FTE without a second
  complete editor face cache. Selected/filtered faces and brush entities still
  receive their required custom surfaces; selection invalidates that cache.
- Selection/hover/locked-overlay candidates are rebuilt when their relevant
  state changes, rather than rediscovered from every brush every frame.
  Camera-dependent culling still runs when drawing the cached candidates.
- Point-entity, container and patch lists are cached by document generation.
  Their per-frame passes no longer walk all 20,000 unrelated world brushes.
- The editor VM reserves 256 MiB of address space, committing/touching memory
  as used. Bounded individual QC allocations support up to 64 MiB, allowing the
  packed caches that previously exceeded FTE's 16 MiB per-block limit.
- Packaging rejects engines missing the indexed-query patch, so an older cached
  engine cannot silently remove the optimization from a new distribution.

Grid size, line thickness, antialiasing, textures, model visibility, geometry,
collision, selection rules and saved MAP data are unchanged. This does not build
a BSP or require VIS to edit a large MAP.

## Measurements and scope

Disposable synthetic MAPs contain 64-unit cuboids spaced 128 units apart. Tests
use a fixed camera followed by a controlled camera strafe, at 1280 x 800 with
the normal grid/edge overlays. On a Windows RTX 4060 Laptop GPU system:

- 3,000 brushes: alpha 73 measured about 107 FPS stationary / 118 moving;
  alpha 74 measured about 304 / 249 in the corresponding runs.
- 20,000 brushes: the initial alpha 73 fixture hit the source-line runaway
  limit, so there is no honest alpha-73 FPS comparison at that size. After the
  loading fixes, an intermediate implementation measured about 22 FPS. The
  final cached passes measured roughly 48–62 FPS across verification runs.
- Cropped 3D viewport captures for alpha 73 vs alpha 74 at 3,000 brushes were
  pixel-identical. Intermediate vs final 20,000-brush captures also matched.

These are fixture results, not a guaranteed frame rate for every real map.
Shader complexity, patches, transparent surfaces, entity counts, GPU and display
resolution still matter. Linux SDL2 also passed the 20,000-brush checks under
Xvfb/software rendering; those FPS numbers are not comparable to hardware OpenGL.

## Validation

- `tests/test-brush-index.py --engine PATH/engine`: compiles the actual patched
  cache and checks 240,000 exact queries, misses, eight-model cache eviction,
  deletion/swapping, same-count replacement, reallocation, duplicate IDs and
  freed/reused models.
- `tests/benchmark-large-map.py --verify --output NEW_DIRECTORY --engine ENGINE
  --host PLUGIN`: generates 20,000 brushes without reading user maps. Verifies
  brush/line counts, typed lists, exact picking, selection cache invalidation,
  geometry edits and Undo/Redo lookup after the measurements.
- Crumble/brush-entity render-tree, grouping and Undo/Redo regression checks.
- Grid depth/occlusion regression captures and archive integrity checks.

## Check on your own map

1. Open a copy of the large MAP and compare camera flight with alpha 73, using
   the same viewport size, grid, antialiasing and texture-filtering settings.
2. Check white contours and surface grid, especially near the camera, at viewport
   edges and after changing grid size/rotation. Repeat in 2D and split layouts.
3. Select/deselect, Shift-hover a face, move/rotate a brush, then Undo/Redo.
   No old outlines or surfaces should remain at the previous position.
4. Hide/show and lock/unlock layers/groups; check point entities, brush entities,
   transparent materials and patches. Their visibility must match the old build.
5. Save/reopen a copy and check that geometry and texture settings are preserved.

All automated fixtures and exports live in disposable directories. User maps,
game directories and mod archives are not rewritten by these tests.
