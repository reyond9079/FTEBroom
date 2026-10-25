#!/usr/bin/env bash
set -euo pipefail
src=$(cd "$(dirname "$0")/.." && pwd)
deps=${1:?Dependency build root}
test=$(mktemp -d "$deps/linux38-XXXXXX")
cp -a "$src/runtime" "$test/ftebroom"
cp "$src/runtime/default.fmf" "$src/runtime/installed.lst" "$test/"
cp "$src/csaddon.dat" "$test/ftebroom/"
cp "${REYONDBROOM_TEST_ENGINE:-$deps/fteqw/engine/release-linux/fteqw-gl64}" "$test/fteqw-gl64"
cp "$deps/fteplug_ftebroom_amd64.so" "$test/"
cat > "$test/ftebroom/linux38.cfg" <<'CFG'
set ftebroom_skip_setup 1
exec ftebroom.cfg
in 8 ftebroom_test_desktop linux38
in 10 screenshot russian-cursor.png
in 12 quit force
CFG
cd "$test"
env -u WAYLAND_DISPLAY LIBGL_ALWAYS_SOFTWARE=1 timeout 45s xvfb-run -a -s '-screen 0 1280x800x24' ./fteqw-gl64 -ftebroom-child-processes -nohome -noupdates -window -width 1280 -height 800 -condebug +exec linux38.cfg > console.txt 2>&1
if grep -E "DESKTOP_FAIL|CSQC_Abort|not a zip|No archive in|couldn't exec (config|fte).cfg" console.txt; then exit 1; fi
for check in cyrillic_all_font_scales linux_native_cursor_owner linux_cursor_fallback_generated linux_cursor_shape_text linux_cursor_shape_ew linux_cursor_hidden_for_camera linux_native_cursor_after_camera; do grep "DESKTOP_PASS $check" console.txt; done
echo "PASS $test"
