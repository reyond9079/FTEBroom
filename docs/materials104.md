# Classic MAP materials and missing compile textures (alpha.104)

## Quake II

`__TB_empty` is a valid editor placeholder, not a game texture. It now has an original neutral 64x64 image in the material catalogue and a generated WAL in compile snapshots. MAP spelling is retained; material lookup is case-insensitive. PCX and standalone true-colour images are indexed alongside WAL files. When a WAL exists its logical dimensions remain authoritative.

Classic Quake 1, Quake II and Half-Life MAP brushes use the same cached, unlit material pass for world, groups, selection and drag previews, with optional directional face shading. This prevents native lightmap shader state from making only some editable brushes appear fullbright. Real lightmaps in compiled BSP and GPU MAP previews are unchanged. Material caches are retained across selection changes.

Quake II WAL files used by a snapshot are copied from FTE's mounted resources into that snapshot's `textures/` directory. This also makes rerelease fallback resources available to the compiler without modifying classic baseq2 or the selected mod. Original WAL data and mip chains are preserved. Generated/default material pixels use the active game palette.

Read-only PAK resource discovery can skip non-canonical member names found in official Q2 archives. It still checks offsets and rejects traversal requests; archive rewriting retains strict validation.

## Missing materials during compilation

When exporting current contents for a compiler or GPU MAP bake, the editor checks the textures actually used. If any are unavailable it pauses before launching tools and opens **Missing compile textures** with the exact names:

- **Create placeholders and continue** stages visible checkerboards under the original material names.
- **Cancel**, including the window close button, stops that operation so resources can be reconnected.

WAD2 is generated for Quake 1, WAD3 for Half-Life, WAL for Quake II and TGA for Quake III/FTE Custom game. Placeholders and a `placeholder-textures.txt` report are written into the compile working copy. Source MAPs, game directories and original WADs are not changed. GPU baking uses the staged placeholder pixels too. Repeating a compile asks again while the original assets are still missing.

An absent WAD no longer blocks a build when all textures actually used by the map are available from other connected WADs. Existing original textures keep their normal precedence and mip chains. Malformed archives and invalid material names remain errors; placeholders do not repair corrupt geometry or invalid formats.

## Brightness

**Preferences > View > Brightness** and **Map > View options > Brightness** expose display gamma, contrast and a reset button. The controls affect display output, including the interface; they do not modify texture pixels, saved MAPs, compiler settings or GPU irradiance. Preferences are retained after restart.

## Verification

The local `sl_1_02.map` copy contained 27,959 brush faces. 11,359 used `__TB_empty`; 30 custom material names could not be found in the available mounted resources. Real original assets and missing assets are distinguished, rather than describing generated checkerboards as recovered originals. Retail maps and assets are not included in the distribution.

Regression fixtures cover WAD2/WAD3/WAL round trips, palettes, mip chains and safe material paths. Disposable editor tests cover Q2 material lookup, full snapshot staging, missing-resource consent, GPU MAP baking, cancellation and native compilation. The original maps are checked unchanged.
