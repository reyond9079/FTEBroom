#!/usr/bin/env bash
set -euo pipefail
src=$(cd "$(dirname "$0")/.." && pwd)
deps=${1:?Dependency root}
test=$(mktemp -d "$deps/sdl70-XXXXXX")
cp -a "$src/runtime" "$test/ftebroom"
cp "$src/runtime/default.fmf" "$src/runtime/installed.lst" "$test/"
cp "$src/csaddon.dat" "$src/tests/resolution70.cfg" "$test/ftebroom/"
cp "$deps/fteqw/engine/release-sdl2/fteqw-glsdl2_64" "$test/fteqw-gl64"
cp "$deps/fteplug_ftebroom_amd64.so" "$test/"
cd "$test"
env -u WAYLAND_DISPLAY SDL_VIDEODRIVER=x11 LIBGL_ALWAYS_SOFTWARE=1 timeout 45s xvfb-run -a -s '-screen 0 1600x1000x24' ./fteqw-gl64 -ftebroom-child-processes -nohome -noupdates -window -condebug +exec resolution70.cfg > console.txt 2>&1
if grep -E 'DESKTOP_FAIL|CSQC_Abort|Error:' console.txt; then echo "$test";exit 1;fi
for check in resolution70_apply resolution70_framebuffer resolution70_document_preserved resolution70_imgui_restored; do grep "DESKTOP_PASS $check" console.txt; done
grep 'vid_width.*1280' console.txt
grep 'vid_height.*800' console.txt
echo "PASS SDL2 resolution restart $test"
