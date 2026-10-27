# Alpha 83: large-map navigation and startup reliability

## Rendering changes

A retained spatial hierarchy rejects fully hidden branches before visiting their
individual faces, grids, entities and container bounds. Leaves retain the existing
visibility checks. Wide walls whose bounding spheres cross the camera are now
eligible occluders when their actual vertices are safely in front of it. Triangles
crossing the near plane remain excluded from the occlusion proof. Projected tile
bounds are clipped before integer conversion.

Integer radix ordering replaces repeated comparison sorting. Generated transparent
helper draws can be deferred across opaque grouping only when FTE already places
them in a later backend sort bucket. Their relative order stays intact; custom
shaders, alpha-tested opaque surfaces and edited-layer insertions remain barriers.
No mesh simplification, texture downgrade, viewport-image cache or geometry draw
distance limit was introduced. Small maps retain the established rendering paths.

The implementation was compared with official TrenchBroom commit
[ad2b95c](https://github.com/TrenchBroom/TrenchBroom/tree/ad2b95c952746fb9ecb53b8d7a339719cfb86a1a).
Its [face renderer](https://github.com/TrenchBroom/TrenchBroom/blob/ad2b95c952746fb9ecb53b8d7a339719cfb86a1a/lib/TbRenderLib/src/FaceRenderer.cpp)
retains geometry and draws by material; its
[text renderer](https://github.com/TrenchBroom/TrenchBroom/blob/ad2b95c952746fb9ecb53b8d7a339719cfb86a1a/lib/TbRenderLib/src/TextRenderer.cpp)
uses a 768-unit default label range and a 128-unit fade. ReyondBroom now follows
that label distance, with selected labels and orthographic views exempt from the
distance rule. The setting persists in Map → View options; `0` means unlimited.
Occlusion still applies to selected labels.

## Correctness fixes

- Startup brush creation could corrupt packed geometry pointers: the 56-byte
  header was advanced in 12-byte variant units, truncating the next offset to 48.
  Offsets now use exact 4-byte words. This also protects incremental large-map edits.
- The entity inspector validates the open-container index before reading it,
  including the startup document's `-1` sentinel, and bounds parent traversal.
- Shader source lookup now hashes names without case, matching its existing name
  comparison. Mixed-case MAP names such as `TechTrim64_b` find lower-case generated
  WAD materials while unselected, as well as in the selected overlay. The MAP and
  original WADs are unchanged.

## Measurement correction (27 October 2026)

The navigation measurements below started outside the enclosing sky/box brushes.
They measured the exterior shell, not the detailed interior. They are retained as
historical exterior-only results and must not be used to claim 60 FPS inside this
map. Interior measurements replace that acceptance criterion: use verified interior
camera positions, show actual room geometry, and record start/middle/end screenshots
and camera coordinates during motion. Keep the real WADs and rendering settings.

## Historical exterior-only measurements

Windows, Core Ultra 7 155H and RTX 4060 Laptop GPU. Read-only E1M1 FTE (2).map:
30,327 brushes, 26,288 entity/container records, 189,772 faces, 35 real WADs and
6,028 WAD texture names. Material browser open, 8x MSAA, 16x anisotropy, textures,
models, outlines, bounds and the previously authorized grid fade retained.
Resolution denotes the complete editor client, including its panels.

The camera starts at `-8000 0 128`, angles `0 0 0`, FOV 90, then actually travels
along +Y. Each sample covers eight seconds of continuous production camera motion.
Final displacement was checked: +5,404 / +5,460 units at 1080p / 1440p.

- **1920×1080: 74.30 FPS** versus alpha.82's 20.13 FPS, about **3.69x**.
  Median frame 13.1 ms; p95 17.6 ms; p99 19.4 ms; maximum 22.11 ms; 595 frames.
- **2560×1440: 68.01 FPS** versus alpha.82's 20.59 FPS, about **3.30x**.
  Median frame 14.3 ms; p95 17.7 ms; p99 19.7 ms; maximum 22.30 ms; 545 frames.

An earlier 1080p run with unlimited labels reached 76.69 FPS. That is a separate
sample, not evidence that more labels improve performance. The major gain comes
from visibility traversal and ordering, not label removal. The final numbers above
use the shipped 768-unit label default and the fixed mixed-case shader lookup.
No compiler, packaging job or separate editor instance ran during final samples.
Linux checks use WSL/Xvfb software rendering and establish functionality, not
comparable Linux GPU FPS.

## Validation

Five 2560×1440 reference/optimized pairs used identical camera poses, textures,
MSAA, anisotropy, grids and unlimited labels. Three viewport pairs were identical;
the other two differed at 1 and 75 pixels respectively, with maximum per-channel
difference 2/255. No missing visible surfaces were found. Animated entity models
were disabled only for these image comparisons, not for navigation measurements.

Production-helper tests cover 30,000-face retained rendering, 12,800 hierarchical
leaf visibility checks, order preservation around transparent helpers, exact
index counts, revisions, access changes, VM relocation, renderer fallback and
resource release. Conservative occlusion tests exercise 47,068 queries including
wide walls and near-plane rejection. Label tests cover fading, selected IDs,
selection changes without a source rebuild, 2D exemptions and cache invalidation.
The shader-key regression checks mixed case across many hash-table sizes.

Startup creation, Undo/Redo and exact packed offsets passed in an isolated Windows
runtime. Linux native and SDL2 passed small selection/group/link regressions. Both also
passed Create/Undo/Redo/Delete on the 30,327-brush MAP. A test copy translated only
Windows resource paths to Linux mount paths; original geometry/WADs were not edited.
The original MAP and resources remain read-only.
The original MAP SHA-256 remains
`44c5dce383187921da2b47e9b8ce780334008f0597b9577c293846161625ae0e`.

## Checks after installing

1. Start without opening a recent MAP. Draw several brushes; Undo/Redo, select and
   delete them. The 3D viewport must remain active without an internal server error.
2. Open the large E1M1 MAP with its real WADs. Fly along the exterior and through
   interiors, turn quickly, then create/move brushes and drag vertices.
3. Inspect `TechTrim64_b` and other mixed-case materials with nothing selected,
   select/deselect them, save to a separate file and reopen that copy.
4. Check near/far labels, selection exemptions and the saved label-distance option.
5. Check translucent surfaces, grids, bounds, layers, Undo/Redo and MSAA restart.

Measurements are specific to the stated hardware and route. An average above 60
FPS does not imply that every frame stays below 16.7 ms. Large grouped-submodel
changes and genuinely global document rebuilds still use their existing paths.
