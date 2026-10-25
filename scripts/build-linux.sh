#!/usr/bin/env bash
# Ubuntu 24.04 x86-64; see docs/linux.md for build and runtime dependencies.
set -euo pipefail
src=$(cd "$(dirname "$0")/.." && pwd)
deps=${1:?Pass a dedicated dependency/build directory}
mkdir -p "$deps"
deps=$(cd "$deps" && pwd)
checkout() {
  if [ ! -d "$2/.git" ]; then git clone "$1" "$2"; fi
  git -C "$2" checkout --detach "$3"
}
checkout https://github.com/fte-team/fteqw.git "$deps/fteqw" f937b9d88f71fc4429db5fe56c6a98d922711b2e
checkout https://github.com/ocornut/imgui.git "$deps/imgui" 4806a1924ff6181180bf5e4b8b79ab4394118875
checkout https://github.com/bulletphysics/bullet3.git "$deps/bullet3-3.25" 2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5
cmake -S "$deps/bullet3-3.25" -B "$deps/bullet-build" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_OPENGL3_DEMOS=OFF -DBUILD_CPU_DEMOS=OFF -DBUILD_UNIT_TESTS=OFF -DBUILD_EXTRAS=OFF -DBUILD_BULLET3=OFF -DINSTALL_LIBS=OFF
cmake --build "$deps/bullet-build" -j4 --target BulletDynamics BulletCollision LinearMath
for pair in 'fteqw patches' 'imgui imgui-patches'; do
  read -r directory patches <<< "$pair"
  for patch in "$src/$patches/"*.patch; do
    if ! git -C "$deps/$directory" apply --reverse --check --ignore-whitespace "$patch" 2>/dev/null; then
      git -C "$deps/$directory" apply --ignore-whitespace "$patch"
    fi
  done
done
make -B -C "$deps/fteqw/engine" -j4 DEPCC= DEPCXX= BASE_DIR=. RELEASE_DIR=release-linux FTE_TARGET=linux64 FTE_CONFIG_EXTRA='-DNO_SPEEX -DNO_OPUS -DNO_VORBISFILE -DNO_OGG' LINK_FREETYPE=0 gl-rel
make -B -C "$deps/fteqw/engine/qclib" -j4 R_qcc CC=gcc BASE_CFLAGS=-DQCCONLY BASE_LDFLAGS='-s -lz'
"$deps/fteqw/engine/qclib/fteqcc.bin" -src "$src/qc" -srcfile csaddon.src -max_globals 262144
g++ -std=c++17 -O2 -shared -fPIC -pthread -I"$deps/bullet3-3.25/src" -I"$deps/fteqw/plugins" -I"$deps/imgui" "$src/native/ftebroom.cpp" "$deps/imgui/imgui.cpp" "$deps/imgui/imgui_draw.cpp" "$deps/imgui/imgui_tables.cpp" "$deps/imgui/imgui_widgets.cpp" "$deps/bullet-build/src/BulletDynamics/libBulletDynamics.a" "$deps/bullet-build/src/BulletCollision/libBulletCollision.a" "$deps/bullet-build/src/LinearMath/libLinearMath.a" -lz -o "$deps/fteplug_ftebroom_amd64.so"
mkdir -p "$deps/compilers"
python3 "$src/scripts/fetch-q3map2-source.py"
unzip -qo "$src/tools/q3map2/netradiant-custom-20260114-source.zip" -d "$deps/compilers"
unzip -qo "$src/tools/ericw/ericw-tools-v0.18.1-source.zip" -d "$deps/compilers"
python3 "$src/scripts/linux-compiler-compat.py" "$deps/compilers"
make -C "$deps/compilers/netradiant-custom-20260114" -j4 DEPENDENCIES_CHECK=off binaries-q3map2
curl -fL https://github.com/RenderKit/embree/archive/refs/tags/v2.17.7.tar.gz -o "$deps/compilers/embree-v2.17.7.tar.gz"
echo "c0694f287fa1fbb382aba3a895702e248d92f7e89c0e5f64aefee965482ec04c  $deps/compilers/embree-v2.17.7.tar.gz" | sha256sum -c -
tar -xf "$deps/compilers/embree-v2.17.7.tar.gz" -C "$deps/compilers"
cmake -S "$deps/compilers/embree-2.17.7" -B "$deps/compilers/embree-build" -DCMAKE_BUILD_TYPE=Release -DEMBREE_ISPC_SUPPORT=OFF -DEMBREE_TUTORIALS=OFF -DEMBREE_TASKING_SYSTEM=INTERNAL -DEMBREE_MAX_ISA=SSE2 -DEMBREE_STATIC_LIB=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build "$deps/compilers/embree-build" -j4
cmake -S "$deps/compilers/ericw-tools-0.18.1" -B "$deps/compilers/ericw-build" -DCMAKE_BUILD_TYPE=Release -Dembree_DIR="$deps/compilers/embree-build" -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build "$deps/compilers/ericw-build" -j4 --target qbsp vis light
checkout https://github.com/bugaevc/wl-clipboard.git "$deps/wl-clipboard" 3eb912c274042cd5deed6b478b39908a12f37498
if [ ! -f "$deps/wl-clipboard/build/build.ninja" ]; then
  meson setup "$deps/wl-clipboard/build" "$deps/wl-clipboard" -Dbuildtype=release
fi
ninja -C "$deps/wl-clipboard/build"
python3 "$src/scripts/package-linux.py" --deps "$deps" --output "$src/out/linux" --version 0.5.0-alpha.70
