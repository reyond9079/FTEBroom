# Linux x86-64 distribution

The native Linux preview targets **Ubuntu 24.04 or compatible glibc 2.39+ systems**
with an X11 or Wayland desktop and OpenGL. Alpha 40 is also tested in an isolated
Arch Linux userspace with Weston. It is not an AppImage and is not a Windows
binary wrapped in Wine. Extract the complete archive and run `./Launch-ReyondBroom.sh`.
Use a writable installation directory; settings stay with the portable installation.

Install runtime dependencies on Ubuntu 24.04:

```sh
sudo apt install libgl1 libx11-6 libxrandr2 libxcursor1 libxi6 libasound2t64 libpulse0 \
  libpng16-16t64 libjpeg-turbo8 zlib1g libstdc++6 \
  ffmpeg xdg-utils libwayland-client0 python3
```

Q3Map2 and Quake 1 qbsp/vis/light are bundled as native binaries. Q3Map2 has a
private `lib/` directory with its dependencies, licenses and corresponding sources;
do not set that directory globally in `LD_LIBRARY_PATH`. Compiler paths
default to these binaries and remain editable. Sound editing uses the installed
`/usr/bin/ffmpeg`; the package contains a small launcher, not a copied FFmpeg binary.

## Platform differences

- MAP editing, ImGui, profiles, geometry, material/entity browsers, native BSP
  inspection, builds and the internal file manager are available.
- Linux external WAD drag-and-drop is supported on Wayland and X11. Image-to-WAD
  conversion and native Windows file pickers remain Windows integrations.
- **Preview mod** opens an isolated game window on Linux. Escape opens the
  in-process ImGui controls and lighting menu; Continue/Escape resumes play, and
  Exit mod preview returns to editing. Native and SDL2 packages retain their
  respective engine backend. Windows retains embedded child-window preview.
  See [lighting presets and checks](mod-lighting100.md).
- Automatic Linux updates require Python 3 and install after exit, retaining settings
  and rollback files. Alpha 39 and older require a manual upgrade to alpha 40.
  The LibreQuake installer remains Windows-only; download LibreQuake from the
  provided link, extract it and select its `lq1` folder.
- The first Linux package is a preview, not a claim of all-platform feature parity.

## Rebuild

Enable Ubuntu `deb-src` repositories before packaging. The packager downloads the
exact source versions of its compiler-only libraries with `apt-get source --download-only`.
No root access is needed for that download after package indexes are configured.

Install these build dependencies in addition to the runtime dependencies:

```sh
sudo apt install meson libwayland-dev wayland-protocols build-essential git curl ca-certificates python3 cmake pkg-config \
  unzip zip zlib1g-dev libpng-dev libjpeg-dev libfreetype-dev libgl-dev libx11-dev \
  libxrandr-dev libxcursor-dev libxi-dev libasound2-dev libpulse-dev libssl-dev \
  libsdl2-dev libogg-dev libvorbis-dev libopus-dev libspeex-dev liblzma-dev \
  libassimp-dev libxml2-dev libglib2.0-dev
bash scripts/build-linux.sh /path/to/dedicated-build-directory
```

The script pins the same engine/ImGui sources as Windows and applies repository
patches. Compiler source archives are included in the corresponding-source bundle;
`scripts/linux-compiler-compat.py` records the GCC 13 compatibility edits. It does
not change compiler geometry algorithms. Sources and licenses ship in the archive.

## Wayland and Arch Linux

Alpha.39 bundles wl-copy/wl-paste from wl-clipboard 2.2.1, including corresponding
source and GPL license. No separate clipboard utility installation is required.
Clipboard requests run asynchronously, with a 4-second timeout and a 16 MiB limit.
On X11 the editor uses FTE's native clipboard interface. On Wayland it uses the
bundled helper, including when the editor itself runs through XWayland.

The native engine owns the cursor. On Wayland it uses `wp_cursor_shape_v1` when
the compositor advertises it; on X11 it loads the matching Xcursor theme shape.
ImGui's cursor atlas is used only as an image-cursor fallback and is not drawn as
a second pointer. Relative camera mode remains separate from ordinary absolute
pointer movement.

On Arch, install the corresponding runtime libraries with your package manager:
`mesa libx11 libxrandr libxcursor libxi alsa-lib libpulse libpng libjpeg-turbo zlib
gcc-libs ffmpeg xdg-utils wayland python`.
The Ubuntu-built archive targets glibc 2.39+; Arch dependency versions can differ,
so inspect a missing-library error with `ldd` rather than renaming shared libraries.

## SDL2 build (alpha.70)

The optional `Linux-SDL2-x64.tar.gz` archive uses FTE's SDL2 video/input backend.
Extract it into its own directory and run `./Launch-ReyondBroom.sh`. Install
`libsdl2-2.0-0` on Ubuntu or `sdl2` / `sdl2-compat` on Arch, in addition to the
runtime dependencies above. SDL selects Wayland or X11; for troubleshooting use
`SDL_VIDEODRIVER=wayland ./Launch-ReyondBroom.sh` or `SDL_VIDEODRIVER=x11 ...`.
Do not mix the native and SDL archives. Updates retain the selected backend and
wait for a matching SDL release asset instead of installing the native engine.

Build dependencies and the plugin are shared with the native build. After
`scripts/build-linux.sh`, run `bash scripts/build-sdl2-linux.sh /path/to/build`.
The script deliberately uses the generic SDL target with `sdl2-config`, avoiding
the upstream SDL2 target's assumption that all codec libraries were built statically.

New installations default to a 1280 x 800 window. Existing saved engine settings
remain authoritative. Change the framebuffer/window size with **View > Resolution**
or **Edit > Preferences > View > Resolution**. Applying a resolution uses the safe
renderer restart and retains the current MAP document. Return from mod preview
or engine terrain before changing it. Settings persist between launches.
