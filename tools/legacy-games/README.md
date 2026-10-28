# Legacy game toolchains

Exact revisions and upstream download URLs are recorded in `PIN.json`. Quake II
uses an isolated ericw-tools 2.0.0-alpha3 bundle; Quake 1 keeps its existing 0.18.1
bundle. Half-Life uses SDHLT 1.3.1. Preserve the complete SDHLT license, including
its Valve SDK and id Software terms; do not describe it as GPL-only.

Release packages include compiler binaries and shared dependencies, license
notices, and matching source archives as separate Sources downloads. Retail game
assets are not included. FreeHL, FreeCS and Nuclide use their included ISC notices.

## Reproduce

1. Download the pinned ericw-tools Windows/Linux release archives. Clone its exact
   source revision with recursive submodules for the corresponding source bundle.
2. Fetch SDHLT at the pinned revision. Apply `tools/legacy-games/sdhlt-mingw64.patch` from the
   SDHLT source root. Build with CMake on Linux and MinGW64 on Windows. The patch
   fixes portable CRT macros, pointer widths and aliasing; Windows tools link
   their compiler runtime statically. Keep Linux `tools/sdHL*` and Windows
   `tools/sdHL*_x64.exe` outputs together in the prepared source tree.
3. Fetch Nuclide and put the pinned FreeHL checkout in `valve` and FreeCS in
   `cstrike`. In each game's `src`, run `make QCC=/absolute/path/to/fteqcc` using
   the pinned FTE compiler. Build all client, server, HUD, menu and rules modules.
4. Run `python scripts/build-legacy-runtime-packages.py --sdk SDK --output PACKS`.
   The deterministic PK3 packages include runtime resources, compiled programs,
   FGD definitions, exact revisions and notices.
5. Run `python scripts/prepare-legacy-games.py --sdhlt SDHLT --ericw-source ERICW
   --ericw-windows WIN_BIN --ericw-linux LINUX_BIN --sdk SDK --runtimes PACKS
   --msvc MSVC_RUNTIME_DIR`. Supply a redistributable Microsoft VC runtime matching
   the official Windows ericw binaries. The prepared cache is ignored by Git.
6. Run the normal platform packager. It verifies every prepared file's SHA256,
   copies the required platform tools, and separates sources from runtime files.

Generate original test assets with
`python tests/generate-legacy-fixtures.py --output TEST_DIRECTORY` after running
`scripts/generate-legacy-editor-textures.py`. Compile the two `maps/room.map`
fixtures with their game-specific toolchain, then build/run
`tests/legacy-profiles.cpp` with C++17 and zlib, passing repository and fixture
directory paths. Tests must use disposable directories, as archive deletion is
deliberately tested without backup creation.
