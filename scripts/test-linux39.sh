#!/usr/bin/env bash
set -euo pipefail
src=$(cd "$(dirname "$0")/.." && pwd)
deps=${1:?Dependency root}
if [ "${2:-}" != inside ]; then exec xvfb-run -a -s '-screen 0 1280x800x24' bash "$0" "$deps" inside; fi
test=$(mktemp -d "$deps/linux39-XXXXXX")
export XDG_RUNTIME_DIR="$test/session"
mkdir -m 700 "$XDG_RUNTIME_DIR"
export WAYLAND_DISPLAY=ftebroom-test
weston --backend=x11 --renderer=pixman --socket="$WAYLAND_DISPLAY" --width=1280 --height=800 --log="$test/weston.log" > "$test/weston-console.txt" 2>&1 &
compositor=$!
trap 'kill "$compositor" 2>/dev/null || true' EXIT
for i in $(seq 1 50); do [ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ] && break; sleep .1; done
cp -a "$src/runtime" "$test/ftebroom"
cp "$src/runtime/default.fmf" "$src/runtime/installed.lst" "$test/"
cp "$src/csaddon.dat" "$test/ftebroom/"
cp "$deps/fteqw/engine/release-linux/fteqw-gl64" "$deps/fteplug_ftebroom_amd64.so" "$test/"
mkdir -p "$test/tools/clipboard"
cp "$deps/wl-clipboard/build/src/wl-copy" "$deps/wl-clipboard/build/src/wl-paste" "$test/tools/clipboard/"
printf 'Привет Arch\nsecond line' | timeout 8s "$test/tools/clipboard/wl-copy" --type 'text/plain;charset=utf-8'
cat > "$test/ftebroom/linux39.cfg" <<'CFG'
set ftebroom_skip_setup 1
exec ftebroom.cfg
in 8 ftebroom_test_desktop linux39
in 10 ftebroom_test_desktop linux39paste
in 11 ftebroom_test_desktop linux39release
in 17 ftebroom_test_desktop linux39check
in 18 screenshot wayland-clipboard.png
in 23 quit force
CFG
cd "$test"
LIBGL_ALWAYS_SOFTWARE=1 timeout 45s ./fteqw-gl64 -ftebroom-child-processes -nohome -noupdates -window -width 1280 -height 800 -condebug +vid_renderer wlgl +exec linux39.cfg > console.txt 2>&1
if grep -E 'DESKTOP_FAIL|CSQC_Abort' console.txt; then echo "$test";exit 1; fi
grep 'DESKTOP_PASS wayland_external_paste_utf8_multiline' console.txt
for check in linux_native_cursor_owner linux_cursor_fallback_generated linux_cursor_shape_text linux_cursor_shape_ew linux_native_cursor_after_paste; do grep "DESKTOP_PASS $check" console.txt; done
actual=$(timeout 8s "$test/tools/clipboard/wl-paste" --no-newline)
[ "$actual" = 'Ответ из ReyondBroom' ]
echo "PASS $test"
