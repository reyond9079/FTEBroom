# Alpha 76: interim large-map performance update

This desktop preview continues work on the supplied 30,327-brush / 26,288-entity Quake 1 MAP. It is an interim release: **60 FPS in the whole-map view at 1920×1080 and 2560×1440 has not been established.** Further optimization is in progress.

## Changes

- Submit editor surfaces, surface grids and point-entity boxes through native batches. Preserve shared vertices, primitive order, materials, colors and existing index limits.
- Avoid scanning thousands of point entities for every brush-only visibility query. Use a bounded spatial index for network brush models and skip empty spatial-grid cells.
- Index smaller brush models as well as the world. Keep trace indexes within a shared 64 MiB budget, with eviction and edit/delete invalidation.
- Retrace remembered occluders before searching again. Ordinary picking and gameplay physics retain their nearest-hit trace paths.
- In Quake 1 perspective editing, hide entity labels behind visible opaque brush faces and sky. A separate surface index observes hidden objects and material filters; fences, liquids, translucent and helper materials do not become opaque occluders. Other profiles keep the existing visibility path.
- Increase the editor VM memory allowance from 256 to 512 MiB for large documents and their caches.

There is no distance cutoff, geometry simplification, reduced texture quality or interface redesign. An experimental geometry-occlusion path failed the visual comparison and is excluded from this release. Only label visibility changes intentionally.

## Validation

Automated checks exercise the actual native implementation: buffer bounds, VM argument restoration, identical indexed primitive streams, 16-bit batch boundaries, trace-index eviction and invalidation, and spatial-query equivalence against exhaustive searches. Label tests cover foreground/background, polygon boundaries, sky, transparent materials, visibility flags and invalid buffers.

The user's MAP is opened read-only during verification; its SHA-256 remains `4d40ec4a51adfcd956e776f86bdf0c9dec2ef9f28aab328029af497b5add8626`. The original 22 external WADs are unavailable, so performance checks use fallback materials and cannot establish the original textured appearance or its complete material workload. Linux engine tests using software rendering are correctness checks, not hardware FPS measurements.

## What to check

1. Open a large MAP and verify that all geometry and entities load. Compare navigation inside rooms and the full-map view from outside.
2. Check grid, white brush outlines, transparent materials and visible point models. They should retain their appearance.
3. Move around walls and sky boundaries: hidden entity labels should disappear and become visible again when exposed. Hide a group or change a material filter and check visibility again.
4. Move, resize, delete and undo a brush. Check that labels update immediately, without stale occlusion.
5. Switch between perspective and 2D views, then between Quake 1, Quake III and FTE Custom game profiles.
6. On Linux, test the native and SDL2 packages separately. Reuse existing settings and verify startup, map opening and normal editing.

For diagnosis, `ftebroom_native_overlays 0` restores the QC overlay path, and `ftebroom_occlusion_culling 0` restores the previous label-visibility path. These switches are diagnostic comparisons; they are not required for normal use.

Runtime packages and corresponding source archives are published separately. Existing projects and game resources are not bundled into the release.
