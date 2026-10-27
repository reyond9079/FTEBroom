#!/usr/bin/env bash
# Build after scripts/build-linux.sh has prepared the patched engine and plugin.
set -euo pipefail
src=$(cd "$(dirname "$0")/.." && pwd)
deps=${1:?Pass the prepared dependency/build directory}
# The generic SDL target uses SDL2 without the SDL2 target's makelibs-only static dependencies.
make -B -C "$deps/fteqw/engine" -j4 CPUOPTIMIZATIONS=-O3 FTE_TARGET=SDL SDLCONFIG=sdl2-config FTE_FULLTARGET=sdl2_64 RELEASE_DIR=release-sdl2 DEPCC= DEPCXX= BASE_DIR=. FTE_CONFIG_EXTRA='-DNO_SPEEX -DNO_OPUS -DNO_VORBISFILE -DNO_OGG' LINK_FREETYPE=0 gl-rel
python3 "$src/scripts/build-plugins.py" --output "$deps/plugins"
python3 "$src/scripts/package-linux.py" --deps "$deps" --output "${2:-$src/out/linux-sdl2}" --version "${3:-0.5.0-alpha.83}" --sdl2
