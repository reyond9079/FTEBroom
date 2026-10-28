# Alpha 88: large entity-group selection and direct QuakeC queries

## Changes

- The ImGui inspector queries the editor CSQC VM through FTE's plugin interface.
  Selected entity properties, class usage, map colors, definition path and linked
  group status no longer require a selection file or polling their transport
  cvars with the updated QC package. Settings remain cvars. Older QC packages
  retain the compatibility path; this is an incremental migration, not removal
  of every existing editor cvar.
- The native adapter resolves the current VM/function for each request, copies
  returned strings immediately and restores call registers after queries,
  including when VM globals relocate. Requests run on the engine thread and
  reject nested calls and embedded mod-preview queries. Editing commands remain
  queued through the existing transaction path rather than mutating QC mid-draw.
- Inspector data is read in bounded batches after its epoch changes. Document
  class usage/colors are reused across selection changes. Property aggregation,
  protected keys and browser class selection are cached, and a collapsed Sound
  preview does no selected-property scan.
- Linked-group peers are indexed by ID and document order. Groups with no other
  instance are excluded from the draw search. The E1M1 test document has 448 linked
  IDs but no actual peers: the old selected-group searches repeatedly scanned
  the entire document and produced no lines.
- Static selection bounds and the non-point overlay candidate list are cached.
  Active geometry previews keep the replacement-aware path. Point-box colors
  are prepared in one native call, preserving the script's exact vertex bytes.
  Large selected point overlays conservatively reject offscreen boxes, without
  hiding selected objects behind walls. Small-map selection takes its existing
  path. Materials, grid, antialiasing and texture filtering are unchanged.

## Measurements

Windows, Core Ultra 7 155H / GeForce RTX 4060 Laptop GPU, unlimited frame rate,
VSync off. The owned E1M1 copy contains 30,327 brushes / 189,772 faces and 22,880
selected misc_drip entities with unique origins. Actual WAD resources were
loaded. Camera starts inside the enclosing map at (-3500, -3500, 512), looking
across the level geometry and selected entity plane. Flight alternates lateral
movement inside the enclosure. Each measured interval is eight seconds after
initial loading; no compilation ran during these intervals.

- Alpha 87, 1920 x 1080 selected group: **1.05 FPS** (9 frames in the interval).
- Alpha 88, 1920 x 1080 selected group: **55.70 FPS**, repeat **63.00 FPS**.
- Alpha 88, 1920 x 1080 selected flight: **55.27 FPS**, repeat **56.63 FPS**.
- Alpha 88, 2560 x 1440 selected group: **60.66 FPS**.
- Alpha 88, 2560 x 1440 selected flight: **60.30 FPS**.

The 1080p repeat's flight median frame time was 16.3 ms, p95 22.4 ms, p99 31.8 ms,
with one 196 ms outlier. These results demonstrate a major improvement over the
selection slowdown, but do not establish a stable 60 FPS floor. Different
resolutions/runs vary with workload and system scheduling; the higher 1440p
result does not mean raising resolution improves performance.

The container/link drawing stage fell from about 713 ms/frame to 1.3-1.4 ms/frame.
QC inspector publication during selection fell from 176 ms to 8 ms. The latter
is the QC publication stage, not the whole click-to-inspector latency: native
snapshot parsing and property aggregation are separate work.

## Validation

- All 22,880 selected IDs remain valid and selected; direct queries return all
  22,880 misc_drip property records.
- Compare native point colors against script-generated box vertices: 32 points,
  zero differing words; compare cached bounds against a full selected-point union.
- Compare linked-peer membership and order against the full-scan reference on a
  small fixture; verify linked Duplicate, Move, rotated synchronization, protected
  properties and Undo/Redo, including peer-index invalidation.
- Exercise native property-cache invalidation for selection, definitions, defaults,
  protected keys, mixed values and map changes.
- Exercise VM register restoration, relocated globals, copied temporary strings,
  missing VM/function, nested-call rejection and mod-preview isolation.
- Windows small-map group selection, guide rays and renderer restart pass.
  Both Linux builds pass the same selection/linked-group, direct-query, color and
  renderer-restart checks under virtual Arch / Plasma 6.8.1 / KWin Wayland.
  Linux uses software rendering in this testbed; these are lifecycle checks,
  not physical Linux GPU performance measurements.
- Reconstruct the tested engine source from the alpha 87 baseline plus the new
  patch and check package/source consistency and archive checksums.

## Please check

1. Open E1M1 with its WADs, select the large misc_drip group and fly around it.
   Compare responsiveness and check that every member remains selected.
2. In Entity, check classname, mixed origins, defaults and browser selection;
   change a shared property, then Undo/Redo. Collapse and reopen the panel.
3. Select one entity, deselect it, open another map and return. Ensure inspector
   properties, usage counts and selected-class highlighting are not stale.
4. Move/Duplicate grouped entities and brushes; check bounds, guide rays and
   Undo/Redo. Check linked groups still draw their connections and synchronize.
5. Check one and four viewports, offscreen selections, transparent materials,
   locked objects and small-map outlines. Restart the renderer and reopen a map.
6. On Linux native and SDL2, repeat selection and inspector edits. Report GPU,
   resolution and frame-time spikes if performance still drops.
