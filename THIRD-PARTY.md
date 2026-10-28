# Third-party notices

Release binaries and corresponding sources are separate downloads. Publish the matching
`ReyondBroom-<version>-<platform>-Sources.zip` alongside every binary asset. It contains
the pinned FTE, Bullet, ImGui and compiler sources, editor changes and build scripts.
License notices remain in the runtime package.

- FTE Quake and csaddon: https://github.com/fte-team/fteqw — GPL-2.0-or-later. Copied csaddon sources preserve original notices; see FTE-LICENSE.txt. Changes: ReyondBroom command routing, box creation and Redo dispatch fix.
- Dear ImGui: https://github.com/ocornut/imgui — MIT, Copyright (c) 2014-2025 Omar Cornut. The build copies its complete license into the output.
- Terrain and convex bevel algorithms in `qc/fb_land.qc` and `qc/fb_bevel.qc` are adapted from ReyondBroom revision `08e26168364629c968875215803a4d9720714223`, respectively `TerrainGeneratorDialog.cpp` (Copyright 2026 Kristian Duske) and `BrushBevel.cpp` (Copyright 2026 TrenchBroom contributors), GPL-3.0-or-later. The modified QuakeC implementations preserve these notices. No Qt application is included. See GPL-3.0-LICENSE.txt. The combined editor distribution uses GPL-3.0-or-later; existing GPL-2.0-or-later components retain their original notices and licensing options.
- Noto Sans CJK JP Regular: SIL Open Font License 1.1. ReyondBroom distributes a renamed UI subset; source revision, checksum, regeneration instructions and complete license are in `runtime/fonts`.
- FFmpeg 8.0.1 and LAME are separate LGPL audio tools. The Windows package includes complete corresponding source archives, notices and the build recipe; see `tools/audio/BUNDLE.md` and `scripts/build-audio.sh`.
- GCC runtime components statically linked into the plugin are covered by their licenses and GCC Runtime Library Exception. See https://www.gnu.org/licenses/gcc-exception-3.1.html.

The default build compiles the pinned FTE source with the included patch. An optional user-supplied FTE binary may differ from the pinned SDK. Preserve corresponding engine source and all applicable third-party notices when distributing that binary; do not label the SDK checkout as its corresponding source unless it matches.

The default engine build applies patches/preserve-brush-surface-metadata.patch to the pinned source. Bundled source archives include the base source; the adjacent patch records the engine modification. PNG, JPEG and zlib libraries retain their included upstream notices.

- Bundled q3map2: NetRadiant-custom 20260114, GPL; unmodified compiler, corresponding source archive and runtime notices in [tools/q3map2](tools/q3map2/README.md).

- Quake 1 compiler bundle: ericw-tools v0.18.1 (separate GPL-3.0 tools), unmodified Windows binaries. Version, source URL and checksums: [tools/ericw/PIN.json](tools/ericw/PIN.json). The distribution contains corresponding source, GPL text and Embree/TBB notices. The repository fetches the pinned bundle during build rather than storing a second copy of all binaries.
- Quake entity descriptions in `runtime/quake1.ent`: Copyright (C) 1996-1997 Id Software, Inc., GPL-2.0-or-later; derived from [Quake-Tools qcc/v101qc](https://github.com/id-Software/Quake-Tools/tree/master/qcc/v101qc). ReyondBroom adds structured editor properties and model hints. The original game art, sounds and executable game logic are not included. `quake1-editor.wad` is original ReyondBroom test/editor artwork under the repository license.

- `runtime/librequake.def`: QUAKED descriptions from LibreQuake v0.09-beta (`qcsrc`, https://github.com/lavenderdotpet/LibreQuake/tree/v0.09-beta/qcsrc), Copyright Id Software and LibreQuake contributors, GPL-2.0-or-later. The optional downloader retrieves the original upstream Mod Lite archive and preserves its per-resource license notices; game data is not bundled with ReyondBroom.

- Bezier Bend path interpolation in `qc/fb_bend.qc` follows UltimateTrenchBroom `BezierBendTool.cpp`, revision `e093dc9cd41c2e9beb00a9ed0d0dfa2241e4a666`, Copyright 2026 Kristian Duske, GPL-3.0-or-later. Subtract 2 follows its cuboid grid/fallback workflow. Native controls and QuakeC geometry integration are modified implementations.

Polygon Cut/Spawn and symmetry behavior were studied against UltimateTrenchBroom commit `e093dc9cd41c2e9beb00a9ed0d0dfa2241e4a666`, especially PolygonCutTool.cpp and SymmetryTool.cpp. New QC implementations retain GPL-3.0-or-later licensing; all existing upstream notices remain.

- Quake II compiler bundle: ericw-tools 2.0.0-alpha3 and its Embree/TBB dependencies, isolated from the Quake 1 tools. Exact revision and packages: [legacy tools](tools/legacy-games/PIN.json). Corresponding ericw source with submodules is in the Sources download.
- Half-Life tools: SDHLT 1.3.1, revision d51cb01d0a70d1c7c4b3df3c53dc566a5e7e7011, with the published MinGW portability patch. The complete upstream license includes GPL, Valve SDK and id Software terms; it is preserved with binaries and sources.
- Nuclide, FreeHL (Rad-Therapy) and FreeCS: ISC notices and exact revisions are preserved in the bundled runtime PK3 and corresponding source archive. Original Half-Life/Counter-Strike assets are required separately. The editor installs only its separately named runtime package.
- `runtime/halflife.fgd` and `runtime/quake2.fgd` retain their original entity-definition notices from the TrenchBroom game resource collection. `runtime/counterstrike.fgd` adds editor-side CS1.5 descriptions. `halflife-editor.wad` and `textures/fb_grey.wal` are generated original editor artwork.
