# Alpha 85: retained material rendering and camera-flight stalls

## What changed

The alpha 84 material schedule was rebuilt whenever the camera crossed its
512-unit validity region. Profiling the 189,772-face E1M1 map measured four such
rebuilds at 564–637 ms. This explains the repeated short freezes reported while
flying. Increasing the distance alone would postpone the same expensive work.

Alpha 85 keeps source geometry and draw command lists on the GPU. Eligible static
materials share a bindless texture table; the vertex shader performs the same
source-face plane test, while normal GPU clipping and depth testing determine
visibility. Grid and outline buffers use the same retained access revision.
Camera movement no longer reconstructs those lists or the material schedule.
Opaque primitive order, transparent order, custom-material barriers and edited
brush insertion positions are preserved. Incremental edits update access masks
and render the edited geometry separately, without removing the rest of the map.

Texture copies retain source mip levels, internal formats and sampler settings.
They are invalidated on resource reload, texture upload and filtering changes.
Mutable engine textures never become resident bindless handles. Cached mesh
uploads also preserve foreign VAO bindings and force the correct element buffer,
preventing cached outlines from consuming a later mesh's indices.

The accelerated path currently requires OpenGL 4.3, ARB_bindless_texture and
NV_gpu_shader5. Unsupported drivers and materials keep the established rendering
path. Its conservative legal material schedule now covers the map rather than
rebuilding every 512 units. These RTX results are not a promise for other GPUs.
Private texture copies are capped at 256 MiB; geometry allocations are bounded.
No geometry simplification, texture downgrade, reduced MSAA, geometry distance
limit or reduced small-map detail was introduced.

## Measurement method

Windows, Core Ultra 7 155H, RTX 4060 Laptop GPU. Original E1M1 FTE (2).map,
30,327 brushes, 189,772 faces, 26,288 entity/container records; 35 actual WADs,
about 6,072 browser materials. The fixture supplies 335 of 377 referenced names;
42 missing source textures are a known resource limitation, not substituted
benchmark assets. The original map and WAD files were not modified.

8x MSAA and 16x anisotropy; geometry, models, grid, outlines and entity overlays
enabled. Measurements count presented editor frames over eight-second intervals,
without a stack sampler or simultaneous compilation. Screens were 1920×1080
(3D viewport 1435×803) and 2560×1440 (2080×1163), with the same dock layout.
All positions are **inside** the enclosing map box, looking at actual geometry:
room eye (-1472,-160,296), angles (0,90,0); panorama (-3500,-3500,512),
angles (30,45,0). Short flights move +X at 20 units/s. A longer flight moves
+X at 100 units/s across the former schedule-region boundaries.

## Final camera-flight results

- 1920×1080 room: **95.27 FPS**, p95 13.4 ms, maximum 21.86 ms.
- 1920×1080 panorama: **109.89 FPS**, p95 11.8 ms, maximum 19.55 ms.
- 2560×1440 room: **97.55 FPS**, p95 13.6 ms, maximum 20.84 ms.
- 2560×1440 panorama: **108.30 FPS**, p95 12.3 ms, maximum 15.72 ms.
- Longer 1080p flight, three intervals: **103.32 / 108.71 / 124.28 FPS**;
  maximum frames 20.36 / 19.81 / 18.39 ms.
- Longer 1440p flight: **110.42 / 109.22 / 115.94 FPS**;
  maximum frames 21.85 / 18.21 / 15.66 ms.

The same alpha 84 panorama measured 30.55 and 32.34 FPS respectively: about
3.6× and 3.3× faster in this controlled comparison. The previous half-second
camera-region pauses did not occur in these measured flight intervals. This
is an average-FPS result on one machine, not a 100-FPS minimum guarantee.
Material submission dropped from roughly 544 runs to 18 in the tested view.

## Comparison with unmodified FTE

The official July 13, 2026 Windows binary (same f937b9d engine baseline) ran a
byte-identical raw MAP copy in an isolated folder, with actual extracted WAD
textures and a small read-only camera/frame-count probe. A controlled interior
flight at the same logical viewport measured **131.94 FPS** (p95 9.89 ms,
maximum 13.50 ms). The stock MAP viewer does not perform all editor work or show
all brush entities like ReyondBroom, so this is a diagnostic control rather than
an identical renderer benchmark. Stock stack samples were mainly in the GPU
driver and ordinary terrain/MAP renderer; the editor additionally spent time
preparing faces, grid visibility and legal material order. The optimization
removes that extra camera-dependent work instead of hiding geometry.

## Correctness and remaining limits

Three static interior views were compared against the established material path,
with animated models disabled only for image comparisons. Of 1,152,305 viewport
pixels, 104 differed in the room, 2 in the hall, and 46–47 in the panorama; mean
channel differences were below 0.00006 on a 0–255 scale. Toggling the path twice
produced the same result. Full-map geometry remains present during a valid vertex
preview. Mouse creation, move and vertex commits preserved the base geometry
cache and took approximately 23 / 20 / 22 ms. Full-scan checks confirmed overlay
candidates and material usage after those edits.

Editing is not yet a constant-100-FPS guarantee: this profiled QA run measured
77.54 FPS while moving a brush and 82.95 FPS during a vertex drag. A screenshot
inside the vertex interval caused a 242 ms maximum frame; the creation interval
included a 355 ms transition. These editing outliers are separate from the
resolved repeated camera-region rebuilds and remain visible in the evidence.

Production C harnesses cover retained resources, 30,000-face lists, access
revisions, unsupported materials, transparent order, edited-layer interleaving,
foreign VAO preservation, exact index uploads and conservative visibility.
The large-map compiler export also used a quadratic entity/geometry scan. It now
builds owner buckets once in source order; the complete 31.99 MB export passed
without the previous runaway-loop failure.

Additional runtime checks cover starter-scene creation/undo/redo, seven UI
languages at three scales, resource reload and renderer restart.

Linux native and SDL2 builds use the same engine patch and runtime shaders.
Virtual Arch Linux / KWin Plasma 6.8.1 / Mesa software rendering checks exercise
the fallback and renderer lifecycle. They do not certify physical Wayland input
or the NVIDIA GPU path on a real Arch workstation.
