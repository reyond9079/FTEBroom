# ReyondBroom alpha 75 — entity-heavy MAP performance

## Changes

- NumPad 0 toggles grid visibility, matching the main 0 key. Alt+NumPad 0
  toggles snapping. Neither shortcut consumes keys while typing in a text field.
- Opening a Quake 1 MAP merges its map-relative WAD references with the profile
  archives. A missing archive reports its path without preventing geometry from
  opening or discarding archives that are available. Moved WADs with matching
  filenames can be resolved from the configured mod and texture directories.
  Compilation still reports missing required archives explicitly.
- CSQC submodel capacity supports all brush entities in the supplied large MAP.
  Brush faces that fail float clipping get a bounded double-precision retry;
  ordinary successful faces keep the existing fast path and coordinates.
- Large cache initialization is staged across frames without disabling the VM
  runaway guard. The complete supplied MAP opens with 30,327 brushes and
  26,288 entities; its file hash remains unchanged.
- Editor face submission and conservative entity frustum culling run in native
  code through versioned, range-checked QC buffers. Camera-independent point
  and container data is cached. Entity edits, document changes, visibility,
  selection and filters invalidate the relevant snapshots. Active transforms
  retain the interactive path.
- Point solids and bounds use separate batched passes. Animated model bounds
  are not rejected using smaller entity-definition boxes.
- Main-thread brush traces use a bounded spatial index with stable candidate
  ordering. Edits, removals, swapped entries and destruction invalidate it;
  worker queries retain their existing path.
- Label visibility uses a private any-hit trace. A bounded endpoint cache
  remembers a previously blocking network entity, but retraces its current
  geometry and transform before accepting it. Misses run the complete query.
  Ordinary picking, physics and gameplay traces retain nearest-hit behavior.
- Engine patch ordering was corrected for clean builds. Rebuilding engines
  does not reuse objects compiled against older patched headers.

There is no distance cutoff, hidden class suppression or automatic reduction
in grid, edge, label, entity, model or brush detail. No interface redesign is
included.

## Measured performance

The supplied `E1M1 FTE.map` has 30,327 brushes and 26,288 entities, including
22,880 `misc_drip` entities. On the Windows RTX 4060 Laptop GPU test machine,
1280 x 800, VSync and frame caps disabled, with grid, edges, class names, point
bounds, group bounds and brush-entity bounds enabled:

- A stationary eight-second measurement: **67.9 FPS** average.
- A controlled camera-strafe measurement: **69.2 FPS** average.
- Label visibility work fell from roughly 49 ms to 3–4 ms per frame.
- Tested viewport crops were pixel-identical with the native face pass enabled
  versus its QC fallback, and with cached versus uncached point rendering.

These are averages from the specified camera route, not a guaranteed minimum
for every viewpoint, resolution or computer. The original 22 external WADs
were unavailable; geometry and overlays were tested with fallback materials.
Original texture appearance and the cost of those materials cannot be verified.
Linux native and SDL2 passed full-map opening and trace equivalence under
Xvfb/software OpenGL; those FPS figures are not hardware-rendering benchmarks.

## Validation

- Windows, Linux native and Linux SDL2: complete original brush/entity counts,
  external document path, cache warmup, missing-WAD diagnostics and preservation
  of a usable WAD alongside missing archives.
- Each platform: 2,048 visibility queries, including camera movement, clear
  lines and starts inside geometry, match full nearest-hit trace visibility.
- Brush broadphase: 49 million reference comparisons, stable ordering,
  edit/delete invalidation, worker fallback and bounded cache eviction.
- Native draw/culling ABI: empty buffers, range/stride guards, frustum rejection,
  model exceptions and restoration of QC argument storage.
- Double-precision fallback: normal clipping stays byte-identical, valid thin
  faces survive, malformed/empty inputs remain bounded.
- Windows visual suite: 15 state checks, nine images, selection/locked colors,
  patches and entities; the complete test MAP stays byte-identical.

## What to check

1. Open your large MAP and navigate both inside and outside it. Check FPS at
   your usual resolution, including camera movement and dense entity areas.
2. Select, move, hide, lock and delete brushes and entities, then Undo/Redo.
   Change class-name/bounds/model visibility; confirm immediate updates.
3. Check that labels and bounds remain hidden by opaque brushes. Open a small
   MAP afterward and check that nothing from the previous scene remains.
4. Open a Quake 1 MAP with external WAD references. Reconnect missing WADs and
   verify texture appearance; build with the actual archives available.
5. Try NumPad 0, Alt+NumPad 0 and typing 0 into a text field, on both operating
   systems. Check orthographic views and active move/rotate previews too.
