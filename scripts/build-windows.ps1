param(
    [string]$Engine,
    [string]$MinGW = 'C:/msys64/mingw64/bin',
    [string]$Output = "$PSScriptRoot/../out"
)
$ErrorActionPreference = 'Stop'
$src = (Resolve-Path "$PSScriptRoot/..").Path
& "$PSScriptRoot/fetch-ericw.ps1"
$enginePath = if ($Engine) { (Resolve-Path $Engine).Path } else { $null }
$env:PATH = "$MinGW;$env:PATH"
New-Item -ItemType Directory -Force $Output | Out-Null
$out = (Resolve-Path $Output).Path
$deps = Join-Path $out 'deps'
New-Item -ItemType Directory -Force $deps | Out-Null
function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed: $LASTEXITCODE" }
}
function Checkout([string]$Url, [string]$Folder, [string]$Revision) {
    if (!(Test-Path "$Folder/.git")) { Run git @('clone', '--no-checkout', $Url, $Folder) }
    & git -C $Folder cat-file -e "$Revision`^{commit}" 2>$null
    if ($LASTEXITCODE -ne 0) { Run git @('-C', $Folder, 'fetch', '--depth', '1', 'origin', $Revision) }
    Run git @('-C', $Folder, 'checkout', '--detach', $Revision)
}
$fte = Join-Path $deps 'fteqw'
$imgui = Join-Path $deps 'imgui'
$bullet = Join-Path $deps 'bullet3-3.25'
Checkout 'https://github.com/bulletphysics/bullet3.git' $bullet '2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5'
Run cmake @('-S', $bullet, '-B', "$deps/bullet-build", '-G', 'MinGW Makefiles', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_SHARED_LIBS=OFF', '-DBUILD_BULLET2_DEMOS=OFF', '-DBUILD_OPENGL3_DEMOS=OFF', '-DBUILD_CPU_DEMOS=OFF', '-DBUILD_UNIT_TESTS=OFF', '-DBUILD_EXTRAS=OFF', '-DBUILD_BULLET3=OFF', '-DINSTALL_LIBS=OFF')
Run cmake @('--build', "$deps/bullet-build", '-j8', '--target', 'BulletDynamics', 'BulletCollision', 'LinearMath')
Checkout 'https://github.com/fte-team/fteqw.git' $fte 'f937b9d88f71fc4429db5fe56c6a98d922711b2e'
Checkout 'https://github.com/ocornut/imgui.git' $imgui '4806a1924ff6181180bf5e4b8b79ab4394118875'
foreach ($patch in Get-ChildItem "$src/imgui-patches/*.patch") {
    & git -C $imgui apply --reverse --check --ignore-whitespace $patch.FullName 2>$null
    if ($LASTEXITCODE -ne 0) { Run git @('-C', $imgui, 'apply', '--ignore-whitespace', $patch.FullName) }
}
foreach ($patch in Get-ChildItem "$src/patches/*.patch") {
    & git -C $fte apply --reverse --check --ignore-whitespace $patch.FullName 2>$null
    if ($LASTEXITCODE -ne 0) {
        Run git @('-C', $fte, 'apply', '--check', '--ignore-whitespace', $patch.FullName)
        Run git @('-C', $fte, 'apply', '--ignore-whitespace', $patch.FullName)
    }
}
if (!$enginePath) {
    # Dependency generation is disabled below; included engine patches need rebuilding.
    (Get-Item "$fte/engine/common/plugin.c").LastWriteTime = Get-Date
    $env:FTEBROOM_ENGINE_SRC = "$fte/engine"
    $env:FTEBROOM_GIT_DIR = (Split-Path (Get-Command git).Source).Replace('\','/')
    Run 'C:/msys64/usr/bin/bash.exe' @('-lc', 'export PATH="/mingw64/bin:/usr/bin:$(cygpath -u "$FTEBROOM_GIT_DIR"):$PATH"; cd "$FTEBROOM_ENGINE_SRC" && mingw32-make -B -j8 CPUOPTIMIZATIONS=-O3 DEPCC= DEPCXX= BASE_DIR=. RELEASE_DIR=release-ftebroom FTE_TARGET=win64 FTE_CONFIG_EXTRA="-DNO_SPEEX -DNO_OPUS -DNO_VORBISFILE -DNO_OGG" LINK_FREETYPE=0 gl-rel')
    $enginePath = "$fte/engine/release-ftebroom/fteglqw64.exe"
}
Run 'mingw32-make' @('-C', "$fte/engine/qclib", '-j4', 'R_qcc', 'CC=gcc', 'BASE_CFLAGS=-DQCCONLY', 'BASE_LDFLAGS=-s -lz -Wl,--stack,16777216')
Copy-Item "$fte/engine/qclib/fteqcc.bin" "$out/fteqcc.exe" -Force
Run "$out/fteqcc.exe" @('-src', "$src/qc", '-srcfile', 'csaddon.src', '-max_globals', '262144', '-max_strings', '4194304')
$bundle = Join-Path $out 'ReyondBroom-Windows-x64'
New-Item -ItemType Directory -Force "$bundle/ftebroom", "$bundle/licenses" | Out-Null
Run 'g++' @('-std=c++17', '-O2', '-shared', '-static', "-I$bullet/src", "-I$fte/plugins", "-I$imgui", "$src/native/ftebroom.cpp", "$imgui/imgui.cpp", "$imgui/imgui_draw.cpp", "$imgui/imgui_tables.cpp", "$imgui/imgui_widgets.cpp", "$deps/bullet-build/lib/libBulletDynamics.a", "$deps/bullet-build/lib/libBulletCollision.a", "$deps/bullet-build/lib/libLinearMath.a", '-lz', '-lole32', '-lwindowscodecs', '-lcomdlg32', '-lshell32', '-lgdi32', '-o', "$bundle/fteplug_ftebroom_x64.dll")
Copy-Item $enginePath "$bundle/fteqw64.exe" -Force
Run 'python' @("$src/scripts/build-plugins.py", '--compiler', "$MinGW/g++.exe", '--output', "$bundle/plugins")
Set-Content "$out/launcher.rc" ('1 ICON "'+($fte.Replace('\','/')+'/engine/client/fte_eukaranopng.ico')+'"')
Run 'windres' @("$out/launcher.rc",'-O','coff','-o',"$out/launcher-icon.o")
Run 'g++' @('-std=c++17','-O2','-s','-static','-municode','-mwindows',"$src/native/ftebroom_launcher.cpp","$out/launcher-icon.o",'-o',"$bundle/ReyondBroom.exe")
# Always ship runtime libraries, including dynamically loaded image codecs.
    Copy-Item "$MinGW/zlib1.dll" "$bundle/zlib1.dll" -Force
    Copy-Item "$MinGW/../share/licenses/zlib/LICENSE" "$bundle/licenses/zlib.txt" -Force
    Copy-Item "$MinGW/libpng16-16.dll" "$bundle/libpng16.dll" -Force
    Copy-Item "$MinGW/libjpeg-8.dll" "$bundle/libjpeg.dll" -Force
    Copy-Item "$MinGW/libgcc_s_seh-1.dll", "$MinGW/libwinpthread-1.dll" $bundle -Force
    Copy-Item "$MinGW/../share/licenses/gcc-libs", "$MinGW/../share/licenses/libwinpthread" "$bundle/licenses" -Recurse -Force
    Copy-Item "$MinGW/../share/licenses/libpng/LICENSE" "$bundle/licenses/libpng.txt" -Force
    Copy-Item "$MinGW/../share/licenses/libjpeg-turbo/LICENSE.md" "$bundle/licenses/libjpeg-turbo.md" -Force
    Copy-Item "$MinGW/../share/licenses/libjpeg-turbo/README.ijg" "$bundle/licenses/libjpeg-ijg.txt" -Force
& "$PSScriptRoot/check-windows-runtime.ps1" -Bundle $bundle -MinGW $MinGW
New-Item -ItemType Directory -Force "$bundle/sources", "$bundle/scripts" | Out-Null
Run git @('-C', $bullet, 'archive', '--format=zip', "--output=$bundle/sources/bullet-source.zip", '2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5', 'src', 'CMakeLists.txt', 'BulletConfig.cmake.in', 'UseBullet.cmake', 'bullet.pc.cmake', 'LICENSE.txt', 'VERSION')
Copy-Item "$bullet/LICENSE.txt" "$bundle/licenses/Bullet.txt" -Force
Run git @('-C', $fte, 'archive', '--format=zip', "--output=$bundle/sources/fteqw-sdk-source.zip", 'f937b9d88f71fc4429db5fe56c6a98d922711b2e')
Run git @('-C', $imgui, 'archive', '--format=zip', "--output=$bundle/sources/imgui-source.zip", '4806a1924ff6181180bf5e4b8b79ab4394118875')
Copy-Item "$src/scripts/smoke-windows.ps1" "$bundle/scripts" -Force
New-Item -ItemType Directory -Force "$bundle/tools" | Out-Null
Copy-Item "$src/tools/q3map2" "$bundle/tools" -Recurse -Force
Copy-Item "$src/tools/ericw" "$bundle/tools" -Recurse -Force
if(!(Test-Path "$src/tools/audio/ffmpeg.exe")){throw 'Build the audio helper first: scripts/build-audio.sh'}
Copy-Item "$src/tools/audio" "$bundle/tools" -Recurse -Force
New-Item -ItemType Directory -Force "$bundle/sources/ftebroom" | Out-Null
Copy-Item "$src/qc", "$src/native", "$src/runtime", "$src/scripts", "$src/patches", "$src/imgui-patches", "$src/tests", "$src/screenshots", "$src/docs", "$src/assets", "$src/sdk", "$src/plugins" "$bundle/sources/ftebroom" -Recurse -Force
Copy-Item "$src/*.md", "$src/*LICENSE.txt" "$bundle/sources/ftebroom" -Force
Copy-Item "$src/scripts/build-windows.ps1" "$bundle/sources" -Force
Copy-Item "$src/runtime/*" "$bundle/ftebroom" -Recurse -Force
Copy-Item "$src/runtime/default.fmf" "$bundle/default.fmf" -Force
if (!(Test-Path "$bundle/installed.lst")) { Copy-Item "$src/runtime/installed.lst" "$bundle/installed.lst" }
Copy-Item "$src/csaddon.dat" "$bundle/ftebroom/csaddon.dat" -Force
Copy-Item "$src/FTE-LICENSE.txt" "$bundle/licenses/FTE-GPL-2.0.txt" -Force
Copy-Item "$imgui/LICENSE.txt" "$bundle/licenses/Dear-ImGui-MIT.txt" -Force
Copy-Item "$src/*.md" $bundle -Force
Copy-Item "$src/screenshots", "$src/docs" $bundle -Recurse -Force
Copy-Item "$src/scripts/Launch-ReyondBroom.cmd" $bundle -Force
Copy-Item "$src/scripts/Launch-Checks.cmd" $bundle -Force
$hash = (Get-FileHash $enginePath -Algorithm SHA256).Hash
"Engine origin: $(if ($Engine) { 'user-supplied' } else { 'built from pinned SDK' })`nEngine SHA256: $hash`nSDK/csaddon: f937b9d88f71fc4429db5fe56c6a98d922711b2e`nImGui: 4806a1924ff6181180bf5e4b8b79ab4394118875" | Set-Content "$bundle/build-info.txt"
Write-Host "Built $bundle. Run ReyondBroom.exe. Engine must match the FTE native plugin API."

