"""Package tested binaries with clean runtime defaults and corresponding sources."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile
import runpy

parser = argparse.ArgumentParser()
parser.add_argument("--bundle", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--version", required=True)
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
runpy.run_path(str(source / 'scripts/fetch-q3map2-source.py'))
bundle = args.bundle.resolve()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=False)
root = output / "ReyondBroom-Windows-x64"
root.mkdir()
(root / "bin").mkdir()
if not (bundle/'plugins/rubble/rubble.dll').is_file():
    raise RuntimeError('Build bundled plugins with scripts/build-plugins.py first')
shutil.copytree(bundle/'plugins',root/'plugins')
for name in (
    "ReyondBroom.exe", "fteqw64.exe", "fteplug_ftebroom_x64.dll", "zlib1.dll", "libpng16.dll",
    "libjpeg.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll", "Launch-ReyondBroom.cmd",
):
    if name == "fteqw64.exe":
        engine = bundle / "bin" / name
        if b'ftebroom_reload_asset_models' not in (engine if engine.is_file() else bundle / name).read_bytes():
            raise RuntimeError("Rebuild the engine with model-resource-reload.patch before packaging")
        if b'indexed brush queries enabled' not in (engine if engine.is_file() else bundle / name).read_bytes():
            raise RuntimeError("Rebuild the engine with zz-editor-brush-index.patch before packaging")
        if b'FTEBROOM_REGISTRY_PASS' not in (engine if engine.is_file() else bundle / name).read_bytes():
            raise RuntimeError("Rebuild the engine with model-registry-growth.patch before packaging")
        for symbol in (b'ftebroom_draw_faces', b'ftebroom_cull_points', b'ftebroom_line_visible', b'ftebroom_build_face', b'ftebroom_build_grid', b'ftebroom_incremental_support', b'ftebroom_incremental_bounds', b'ftebroom_pick_candidates', b'ftebroom_collect_marks'):
            if symbol not in (engine if engine.is_file() else bundle / name).read_bytes():
                raise RuntimeError("Rebuild the engine with the alpha81 retained editor passes before packaging")
        shutil.copy2(engine if engine.is_file() else bundle / name, root / "bin" / name)
    else:
        shutil.copy2(bundle / name, root / name)
for name in ("zlib1.dll", "libpng16.dll", "libjpeg.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"):
    shutil.copy2(root / name, root / "bin" / name)
# Transition for alpha.66 and older updaters, which require this exact filename.
# ReyondBroom.exe removes this byte-identical launcher on first start (not the engine).
# Legacy update archives add their compatibility launcher separately.
shutil.copy2(source / "scripts/Launch-Checks.cmd", root / "Launch-Checks.cmd")
shutil.copy2(source / "scripts/Launch-ReyondBroom.cmd", root / "Launch-ReyondBroom.cmd")
shutil.copytree(source / "tools/q3map2", root / "tools/q3map2")
shutil.copytree(source / "tools/ericw", root / "tools/ericw")
shutil.copytree(source / "tools/audio", root / "tools/audio")
if not (root / "tools/audio/ffmpeg.exe").is_file():
    raise RuntimeError("Build the audio helper with scripts/build-audio.sh first")
shutil.copytree(bundle / "licenses", root / "licenses")
shutil.copytree(source / "runtime", root / "ftebroom", ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
for name in ("default.fmf", "installed.lst"):
    shutil.copy2(source / "runtime" / name, root / name)
shutil.copy2(source / "csaddon.dat", root / "ftebroom/csaddon.dat")
tracked = subprocess.check_output(
    ["git", "ls-files", "-z"], cwd=source
).decode("utf-8").split("\0")
for name in filter(None, tracked):
    if name.startswith(("tools/q3map2/", "tools/ericw/", "tools/audio/")):
        continue  # Already included once, with compiler sources and licenses.
    target = root / "sources/ftebroom" / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / name, target)
archives = list((bundle / "sources").glob("*.zip"))
if len(archives) < 2:
    raise RuntimeError("Pinned FTE and ImGui source archives are required")
if not (bundle / "sources/bullet-source.zip").is_file():
    raise RuntimeError("Pinned Bullet corresponding source archive is required")
for archive in archives:
    shutil.copy2(archive, root / "sources" / archive.name)
for path in source.glob("*.md"):
    shutil.copy2(path, root / path.name)
for folder in ("docs", "scripts"):
    shutil.copytree(source / folder, root / folder, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
patches = ", ".join(path.name for path in sorted((source / "patches").glob("*.patch")))
info = (
    f"ReyondBroom {args.version}\n"
    "Engine/SDK: f937b9d88f71fc4429db5fe56c6a98d922711b2e\n"
    f"Engine patches: {patches}\n"
    "ImGui: 4806a1924ff6181180bf5e4b8b79ab4394118875 (imgui-patches/retro-frames.patch, localization.patch)\n"
    "Bullet 3.25: 2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5 (statically linked)\n"
)
for name in ("bin/fteqw64.exe", "fteplug_ftebroom_x64.dll", "ftebroom/csaddon.dat"):
    info += f"{name} SHA256: {hashlib.sha256((root / name).read_bytes()).hexdigest()}\n"
(root / "build-info.txt").write_text(info, encoding="utf-8")
runpy.run_path(str(source / 'scripts/package-sources.py'))['separate_sources'](
    root, output, args.version, 'Windows-x64')
manifest = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in root.rglob("*") if path.is_file()}
(root / "distribution-manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
archive = output / f"ReyondBroom-{args.version}-Windows-x64.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
    for path in sorted(root.rglob("*")):
        if path.is_file():
            zipped.write(path, path.relative_to(output))
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix(".zip.sha256").write_text(f"{digest}  {archive.name}\n", encoding="ascii")
print(f"{archive}\n{archive.stat().st_size} bytes\nSHA256 {digest}")
