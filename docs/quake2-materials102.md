# Quake II and rerelease material fixes (alpha.102)

Quake II rerelease maps may contain `// Game: Quake2RE` and `// Format: Quake2 (Valve)` comments. These now select the Quake II profile automatically. The previous parser did not recognize those names and could retain a Quake 1 profile, with the wrong resource interpretation.

When a selected classic `baseq2` directory has an installed sibling `rerelease/baseq2`, that rerelease directory is mounted as a read-only fallback. The selected classic directory and its PAK/loose assets retain priority. Explicit rerelease selections are unchanged. Custom mod directories retain their existing sibling-baseq2 fallback rule. No game files are copied or edited.

The editor no longer imports its own generated Quake II alias shader as authoritative input over the original WAL files. Reopening resources regenerates aliases from the current mounted resources and palette. Static world aliases explicitly name the same diffuse image used by selected-brush previews.

## Investigation

The supplied MAP declares Quake2RE. Its requested resources were located in the installed official PAK files:

- `e1u1/grate2_5`: classic baseq2 and rerelease baseq2.
- `ctf/color1b2`: CTF and rerelease baseq2; absent in classic baseq2.
- `e1u1/rocks17_1`: Rogue and rerelease baseq2; absent in classic baseq2. The supplied MAP uses this full name, not the unqualified `rocks17_1`.

A fresh Quake II profile rendered a control brush with `e1u1/grate2_5` consistently before and after normal mouse selection, apart from the selection overlay. The source map was copied for testing and the original file retained its SHA256. Resource lookup and profile switching fixes address the reproduced configuration/cache issues; arbitrary custom-mod textures still require the corresponding assets.

## Check

1. Open a Quake2RE MAP from a Quake 1 session and verify the profile changes to Quake II.
2. With classic baseq2 selected and the official rerelease installed alongside it, search all three full material names listed above.
3. Select/deselect a textured brush: texture identity, scale and alignment must remain unchanged beneath the selection overlay.
4. Reload resources and restart; verify the same textures remain available.
5. Verify a custom mod's own assets override its base game's files.
