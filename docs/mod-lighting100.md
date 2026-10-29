# Realtime lighting in mod preview (alpha.100)

Press **Escape** while playing a mod and expand **Lighting**. The same lighting
controls are available in Windows, Linux native and Linux SDL2 builds. Windows
uses the embedded editor viewport; Linux opens an isolated game window with an
in-process Dear ImGui menu. **Continue** or Escape resumes control. **Exit mod
preview** closes the game process and returns to editing.

Enable **FTE-native RTL instead of lightmaps** to import ordinary map lights and
render realtime shadows. The name of this compatibility checkbox is retained;
when baked contribution is nonzero the result is explicitly a hybrid.

## Presets and controls

- **Lightmap-like** (compiled BSP default): reach 1.3x, RTL brightness 0.35,
  directional fill 0.20 and baked light contribution 0.70. This keeps the
  compiler's broad illumination and adds live RTL. It is not realtime GI.
- **Soft RTL only** (uncompiled MAP default): reach 1.6x, brightness 0.80,
  directional fill 0.35, no baked contribution. It broadens illumination but
  cannot recreate light bounced from surfaces behind an occluder.
- **Native RTL**: original reach, brightness 1, directional fill 0, no baked light.
- **RTL distance (map units)** limits distance outside each light's influence.
  The last 20% fades; 0 is unlimited. Directional lights and dynamic flashes are
  unaffected. Changing distance does not reimport lights.
- **Light reach** changes imported light radii. **RTL brightness** changes their
  intensity independently of radius. **Directional fill** blends diffuse
  response toward nondirectional illumination inside the light volume, while
  retaining shadow occlusion. Specular import is disabled for this preview.
- **Baked light contribution** keeps a fraction of compiled lighting. Set it to
  zero for pure RTL. It is unavailable on raw MAP because no lightmaps exist.
  The Lightmap-like preset is also disabled there.

Settings apply without a renderer restart. Compiled and raw preview settings
persist separately in the editor's `mod-lighting.txt` and
`mod-lighting-raw.txt`. Old two-field preferences are accepted. Linux child-menu
changes are passed to the editor through a bounded output message and saved by
its normal settings path. Mod files and original maps are not written.

The preview uses an immutable, private entity source from the selected build
snapshot, or current MAP contents for raw preview. Colors, supported targets and
light styles use FTE's native importer. Switching RTL off restores full baked
lighting, or the normal raw-MAP fullbright fallback. Retained baked light is
static; choose Soft RTL only when fully dynamic lighting matters more than
matching the compiled appearance.

## Small-map validation

Two original 512 x 512 x 256-unit test rooms were compiled with the bundled
ericw qbsp/vis/light and bounce lighting: an unobstructed room, and a room with
an L-shaped occluder. Neutral checker textures, camera, FOV and exposure were
held fixed. Windows NVIDIA OpenGL captures compared lightmaps, native RTL,
soft RTL and five hybrid configurations. The selected default reduced mean
absolute RGB error from 73.89 to 6.56 (room) and 55.37 to 9.44 (occluder) on an
8-bit 0–255 scale, excluding the top 80 pixels used by screenshot notifications.
These are controlled-scene comparisons, not a guarantee of identical lighting
on arbitrary maps. Shadowed surfaces remain visible through baked fill; geometry
still occludes realtime lights. Large-map lighting/performance is not certified
by these tests.

Linux menu checks run in an isolated Arch userspace through WSLg Wayland with
software OpenGL, for both native and SDL2 engines. They exercise the actual
ImGui checkbox, lighting import, Escape/Continue and persisted output messages.
This is not a physical Arch/KDE hardware test.

## What to check

1. Compile a small lit room. In mod preview, Escape should open the lighting
   menu on each OS. Toggle RTL, compare all three presets, and return to lightmaps.
2. Change reach, brightness, directional fill and baked contribution. A wall or
   pillar should still block the realtime component. Baked fill should preserve
   the compiler's indirect illumination; pure RTL can leave deep shadows dark.
3. Set distance to a small value and then 0. Check fading and restoration while
   moving beyond the light's influence.
4. Close preview, reopen it, then restart the editor. Settings should persist.
   Raw MAP should keep its own soft-RTL values, with baked controls disabled.
5. Use Continue, Escape, key rebinding, sensitivity/invert and Exit mod preview.
   There should be one usable cursor in the menu and normal mouse look afterward.
6. Preview a raw MAP with CLIP/trigger surfaces. They should stay invisible and
   collision/trigger behavior should remain. Editing display is unchanged.
