# Experimental GPU lighting (alpha.101)

Open **Setup compile > Experimental GPU lighting** in the Quake 1 profile. Compile Geometry first; VIS and LIGHT can be disabled. Choose 0 rays for direct lighting or 1–64 cosine-weighted hemisphere rays for one diffuse bounce, then **Bake lighting on GPU**. A separate BSP/LIT pair is generated and displayed automatically. The normal compiler remains available.

This prototype uses OpenGL 3.3 on Windows and Linux. A hidden SDL2 context runs the lighting shader in an isolated helper process. It reconstructs lightmap sample positions from BSP planes/texinfo, traces visibility against world triangles, and integrates one reflected-light bounce. The saved lightmaps are camera-independent and need no RTL rendering. Geometry preparation still uses QBSP; baking does not replace BSP or VIS compilation.

## Scope

- Quake BSP29 and BSP2, ordinary static point lights with linear attenuation and optional `_color`.
- Neutral 55% diffuse reflectance for all surfaces. Texture-dependent colour bleeding, sunlight, spotlights, lightstyles, translucent shadow filtering, moving brush submodels and incremental updates are not implemented.
- Maximum 4096 faces, 8192 triangles, 262144 luxels, 128 lights and a bounded estimated ray/triangle work budget. Oversized scenes fail with an explanation before GPU dispatch. This is deliberately a small-map prototype, not a production replacement for ericw.
- Existing output files are refused. Source MAP/BSP, collision, textures and entities are retained. New lightmaps replace lighting only in the new output BSP. Additional BSPX lighting extensions are not copied.
- Unsupported light settings are not equivalent to the compiler. Use only the supported fixtures/settings when comparing results.
- Linux requires SDL2 and desktop OpenGL; the native and SDL2 editor archives include the same helper. GPU baking can contend with viewport rendering while it runs.

## Measurements

Two original 512 x 512 x 256 rooms were tested: an open room and a room containing an L-shaped occluder, using neutral checker textures and one static lamp. Both outputs were opened in FTE from the same camera with RTL disabled. Reference: bundled ericw light 0.18.1, `-extra4 -bounce`. Geometry/VIS time is excluded from both lighting timings.

Windows, RTX 4060 Laptop GPU, 32 bounce rays:

- Open room: 4970 luxels, 100 triangles; GPU dispatch plus readback 0.0033 s; helper startup + bake + write 0.300 s. Compiler LIGHT reported 0.117 s.
- Occluded room: 5422 luxels, 136 triangles; GPU dispatch plus readback 0.0033 s; helper total 0.278 s. Compiler LIGHT reported 0.094 s.
- Thus the complete prototype is **slower on these tiny maps** because context/process startup dominates. No large-map speedup is claimed.
- Mean absolute RGB image difference against the compiler reference (0–255, excluding the top 80 screenshot pixels): 30.93 for the open room, 21.03 for the occluded room. Light distribution is similar but brightness, shadow borders and bounce sampling differ. This is not compiler-quality equivalence.

Arch userspace under WSLg, D3D12 Intel Arc OpenGL backend: open room 0.259 s dispatch/readback, 1.168 s total; occluded room 0.066 s dispatch/readback, 0.468 s total. This is not a physical Arch/KDE benchmark.

Integration checks verify immutable source/geometry lumps, valid RGB LIT length, deterministic output, additional energy from a bounce, rejection of existing output and malformed input.

## Build and test

Windows (MinGW, SDL2 development package):

```
g++ -std=c++17 -O2 tools/gpu-light/main.cpp -o gpu-light.exe -lSDL2 -lopengl32 -static-libgcc -static-libstdc++
```

Ship SDL2.dll and its license alongside the helper. Linux:

```
g++ -std=c++17 -O2 tools/gpu-light/main.cpp -o gpu-light $(sdl2-config --cflags --libs) -lGL
python3 tests/gpu-light.py ./gpu-light /path/to/small-static-world.bsp
```

## User checks

1. Create a small sealed Quake 1 room with a lamp and a solid obstruction; compile Geometry with LIGHT disabled.
2. Bake with 0 rays, then with 32 rays. Behind the obstruction the second output should receive reflected illumination while direct shadows remain.
3. Move the camera: baked illumination must stay attached to the surfaces.
4. Compare against a normal `-extra4 -bounce` compile; check corners, brightness and shadow boundaries.
5. Confirm bake cancellation/unsupported-map errors preserve the previous successful BSP and editor responsiveness.
6. Open the generated BSP with its matching LIT after restarting the editor. Keep the pair together.
