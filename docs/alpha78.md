# Alpha 78: creating brushes on large maps

Creating a brush with the mouse on E1M1 FTE.map exposed two independent failures after the gesture completed:

- The packed grid cache had no spare capacity after loading. Adding the first brush doubled four large arrays and exhausted the 512 MiB editor VM. Large caches now reserve modest headroom and grow by roughly 6.25%, rather than doubling. Small-cache growth stays unchanged.
- Selecting the new brush activated repeated document scans for container selection and entity target links. These could hit the VM runaway guard. Container selection is now aggregated from leaves into their ancestors once per invalidation. Entity links use an index of target names, preserving duplicate names, target/killtarget deduplication and document order. Symmetry guides skip unrelated nodes before inspecting selection. Empty-container cleanup after Delete also aggregates surviving descendants once.

GPU geometry retention and conservative visibility from alpha 77 remain enabled. No draw-distance, detail, material, grid or antialiasing reductions are introduced. Source MAP files are not modified by the regression tests.

## Checks

1. Open the large map and create a cuboid by pressing and dragging the mouse over an existing surface. Keep dragging, then release. The preview and final brush should remain usable.
2. Repeat creation, Undo, Redo and Delete. Check that selection, outlines and surface grids update.
3. Select single brushes and complete groups; check entity target links and symmetry guides.
4. Compare a small map with the previous version and check large-map navigation FPS.

## Validation

- Reproduced the original VM allocation failure after a real brush gesture on the unchanged 30,327-brush MAP, then reproduced the selection/link runaway failures exposed after fixing it.
- The complete create-preview-release-Undo-Redo-Delete sequence passes on Windows, Linux native and Linux SDL2 with the normal 512 MiB VM budget. Counts return from 30,328 to 30,327 as expected.
- Small-map container-selection results match the original reference traversal. Indexed target links match reference edge counts and order, including repeated target names, self references and identical target/killtarget names.
- Small-map group-selection and context Undo/Redo checks pass. Four existing organize-suite failures on this particular fixture also reproduce with the unmodified alpha 77 bytecode; they are not claimed as fixed in this release.
