# GPU lighting directly on MAP (experimental)

Quake 1 brush maps can now receive camera-independent baked lighting without QBSP, VIS, a BSP tree or CPU LIGHT. The normal Compile pipeline remains available and unchanged.

## Use

1. Load a Quake 1 map and its WAD resources. Save the map once to choose an output directory; unsaved edits are included in the bake snapshot.
2. Open **Setup compile > GPU lighting on MAP (no BSP)**.
3. Choose shadow quality: Fast (1 sample), Balanced (4 samples), or Fine (16 samples). Choose 0 bounce rays for direct light only, or 1–64 rays for one diffuse bounce. Balanced and 32 rays are the defaults.
4. Select **Bake MAP lighting**. Preparation and GPU progress appear over the viewport. The isolated helper process can be cancelled; the source MAP and ordinary compiled BSP are never overwritten.
5. The completed MAP preview opens in the same viewport. Use **Back to editing** to return to the original editable map. **Show baked MAP** reopens the result while the editor contents are unchanged. Editing invalidates that preview and requires another bake.

Output is stored in a unique `_ftebroom-build/map-gpu-*` folder beside the source map: `snapshot.map`, `snapshot.rblm`, materials, a checked polygon snapshot and `build.log`. Keep the MAP and RBLM together. The RBLM is a ReyondBroom/FTE extension, not an ericw LIT or a portable BSP lightmap. The original MAP is read only during this operation. A normal compiled BSP is not replaced in build history and remains usable for play/export.

## Implementation

The editor serializes its current contents to an immutable MAP snapshot. FTE asynchronously loads that snapshot without switching the editing world, then exports its exact polygons, texture axes and lightmap grids. Both baking and preview therefore use the same geometry, avoiding numerical mismatches introduced by a MAP round trip.

The OpenGL 3.3 helper builds a spatial BVH over triangles and processes bounded tiles. Opaque shadow rays stop as soon as an obstruction is proven. Alpha-tested materials use their actual texture alpha; translucent materials attenuate/filter rays. Original diffuse textures are not resampled into a low-resolution combined colour texture.

Direct irradiance is calculated first. A second pass traces cosine-weighted hemisphere rays and samples the cached direct irradiance at ray hits, with texture-dependent diffuse colour. This produces one diffuse bounce without evaluating every light again for every reflected ray. Back faces do not reuse front-face irradiance. The result is stored in per-surface lightmaps; FTE packs these into its own MAP lightmap atlases when previewing. Collision geometry is retained, while clip/skip/hint/trigger/origin helpers are hidden in this preview.

## Supported lighting and limits

- Static Quake 1 brush geometry; linear, inverse and inverse-square point lights, constant/inverse-square-offset attenuation, `wait`, `_color`, `_anglescale`, target/mangle spotlights, cone/soft-cone angles and target/mangle sunlight.
- Worldspawn minimum light and its colour; actual texture alpha masks and diffuse colour for a single reflected bounce.
- Animated lightstyles are baked at static brightness. Moving brushes/models are treated as static geometry; external model shadows, patches, surface emitters, projected texture lights, sky-dome lighting and compiler-exact dirt/radiosity are not implemented. This is an experimental preview, not a claim of ericw equivalence.
- Quality differs from ericw: shadow sampling, interpolation, secondary energy and visibility calculations use a different implementation. Compare representative rooms and corners before relying on it for final lighting.
- OpenGL 3.3, a current GPU driver and SDL2 are required. Windows and both Linux packages include the helper. Actual GPU acceleration depends on the driver; a software OpenGL renderer can be much slower.
- Safety budgets: up to one million faces, two million triangles, sixteen million lightmap samples and 4096 lights, plus bounded input/material sizes. Existing outputs are refused. Malformed inputs or unsupported patches fail without replacing the source or the last successful BSP.
- Each bake is a full bake. Incremental relighting and automatic restoration of a bake after restarting the editor are not implemented.

## Verification

`python tests/gpu-map-light.py /path/to/gpu-light` constructs original test geometry and textures without a compiler. It checks light transmission through alpha holes, opaque shadow rejection, identical values at a shared planar seam, preservation of existing output and absence of BSP files. It requires an available desktop OpenGL context.

Measurements and the large-map integration results are recorded in the alpha.103 release notes. Geometry preparation, helper time and result loading are reported separately; GPU-only time must not be mistaken for the entire operation.

### E1M1 integration measurement

Windows, NVIDIA GeForce RTX 4060 Laptop GPU, 30,327 source brushes and 204 lights. The final snapshot contains 189,764 faces and 4,174,359 sampled positions. Balanced 4-sample shadows plus 32 rays for one diffuse bounce:

- GPU dispatch/readback: **205.566 s**.
- Complete helper preparation, bake and file write: **206.875 s**.
- Bake request to completed MAP/RBLM output, including editor snapshot preparation: **233.084 s** (about 3 min 53 s). Viewport loading follows separately.
- All **189,764 / 189,764** face lightmaps matched their MAP snapshot; no BSP was produced. Source MAP SHA-256 was checked unchanged.
- Previous ericw run on this map: LIGHT `-extra4` **1620.307 s**, complete BSP2 + fast VIS + LIGHT **1953.975 s**. These are different lighting implementations/quality settings: ericw used 16 shadow samples and no bounce; the GPU run used 4 and one bounce. The time ratio is not a claim of identical quality.

Windows and Linux regression fixtures verify alpha-hole transmission, opaque/masked shadows, correct sunlight through a sky surface, zero RGB difference at the tested shared planar seam, no reuse of front-face light through a back face, and preservation of existing outputs. Linux software-OpenGL fixture timings must not be compared to the Windows hardware measurement. Large-map screenshots are local verification artifacts; the map and its third-party assets are not distributed.

In the inspected E1M1 room, the broad lighting distribution and texture detail remain comparable, while the vine/window view has a visibly darker background than ericw. These differences are retained as a documented prototype limitation; the alpha cutout itself remains visible and correctly transmits test rays. No whole-map claim of seam-free or compiler-identical quality is made.

Linux native and SDL2 editor builds also completed the full snapshot/bake/preview cycle on the 36-face room fixture. The hardware Linux large-map test uses Arch under WSLg with Mesa D3D12 on the same NVIDIA GPU, not a native Arch/KDE installation. Its reflected-ray pass is substantially slower; a 32-ray trial was stopped before completion and is not reported as a successful timed bake. Windows timings must not be used to predict Linux driver performance. Reduce bounce rays or use direct light only for initial tests on that stack.