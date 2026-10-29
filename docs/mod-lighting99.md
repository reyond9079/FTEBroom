# Mod preview: native realtime lighting

In embedded mod preview, press **Escape**, then open **Lighting**. Enable
**FTE-native RTL instead of lightmaps** to replace baked lightmap contribution
with FTE's world realtime lights and realtime shadows. Disable it to return to
lightmaps (or the normal fullbright fallback for an uncompiled MAP).

**RTL distance (map units)** sets how far away a light's influence volume may be
before it disappears. The last 20% fades smoothly. **0 means unlimited**. A light
whose influence reaches the camera is retained; this avoids darkening nearby
surfaces just because a large light's center is distant. Dynamic muzzle flashes
and directional lights keep their native behavior. Shorter distances reduce
realtime shadow work. This is native realtime lighting, not baked indirect light;
the result can differ from a lightmapped build.

Both settings are applied live and saved in the editor's `mod-lighting.txt`.
There is no renderer restart. Mod configuration, GLSL, WADs, MAP sources and
compiled lightmaps are not rewritten.

## Ordinary map lights

The preview stages a private, entity-only lighting source and lets FTE import
ordinary `light` entities, including their color, intensity, style and supported
target settings. This also handles compilers that strip ordinary lights from the
BSP. Playing the last compiled BSP uses that build's immutable MAP snapshot;
playing the current uncompiled MAP uses its current in-memory contents.
Changing the draw distance does not repeatedly import or rebuild lights.

## Service surfaces in uncompiled MAP preview

Recognized CLIP, player/monster clip, HINT, SKIP, ORIGIN, NULL, AAATRIGGER,
TRIGGER, CAULK, NODRAW and related tool materials use private invisible,
non-shadow-casting shader aliases. Their brush geometry and collision data are
retained. Normal materials named `clipbrick` or `stone/clip` are not hidden, and
sky remains sky. The editor's service-material display is unchanged.

## Verification

- Compile a lit room, enter preview, press Escape and enable RTL. Check lighting
  and realtime shadows; disable RTL and check that the lightmap view returns.
- Change distance, including 0, and move beyond the light's influence. Check
  smooth fading and restoration without changing its nearby radius.
- Edit a light without compiling. Compare last-BSP preview with current-MAP
  preview: each should use its own matching light entities.
- Preview an uncompiled MAP containing clip and trigger brushes. Their surfaces
  should be invisible, without shadows; clip collision and trigger behavior
  should remain. They should still be visible when returning to editing.
- Restart preview/editor and check that lighting preferences are retained.

The Escape menu belongs to the embedded Windows preview. Linux runtime binaries
include the same native lighting importer and distance renderer; existing
restrictions for external Linux mod preview are unchanged.
