"""Package native Linux binaries, pinned corresponding sources and runtime assets."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, tarfile
import runpy
from importlib.util import spec_from_file_location, module_from_spec

p = argparse.ArgumentParser()
p.add_argument('--deps', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--version', required=True)
p.add_argument('--sdl2', action='store_true', help='Package the SDL2 engine instead of the native backend')
a = p.parse_args()
src = Path(__file__).resolve().parents[1]
runpy.run_path(str(src / 'scripts/fetch-q3map2-source.py'))
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
platform = 'Linux-SDL2-x64' if a.sdl2 else 'Linux-x64'
root = out / 'ReyondBroom-Linux-x64'
root.mkdir()
for source, name in [
    (a.deps/('fteqw/engine/release-sdl2/fteqw-glsdl2_64' if a.sdl2 else 'fteqw/engine/release-linux/fteqw-gl64'), 'fteqw-gl64'),
    (a.deps/'fteplug_ftebroom_amd64.so', 'fteplug_ftebroom_amd64.so'),
    (a.deps/'compilers/netradiant-custom-20260114/install/q3map2.x86_64', 'tools/q3map2/q3map2'),
    *[(a.deps/f'compilers/ericw-build/{tool}/{tool}', f'tools/ericw/{tool}') for tool in ('qbsp','vis','light')],
    (a.deps/'compilers/embree-build/libembree.so.2.17.7', 'tools/ericw/libembree.so.2'),
    *[(a.deps/f'wl-clipboard/build/src/{tool}', f'tools/clipboard/{tool}') for tool in ('wl-copy','wl-paste')],
]:
    target = root/name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    subprocess.run(['strip', str(target)], check=True)
    target.chmod(0o755)
spec = spec_from_file_location('compiler_libs', src/'scripts/bundle-compiler-libs.py')
module = module_from_spec(spec)
spec.loader.exec_module(module)
module.bundle_compiler(root/'tools/q3map2/q3map2', a.deps/'compiler-library-sources')
shutil.copytree(a.deps/'compiler-library-sources', root/'tools/q3map2/library-sources')
shutil.copytree(src/'runtime', root/'ftebroom', ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
shutil.copy2(src/'csaddon.dat', root/'ftebroom/csaddon.dat')
for name in ('default.fmf','installed.lst'):
    shutil.copy2(src/'runtime'/name, root/name)
shutil.copy2(src/'scripts/Launch-ReyondBroom.sh', root/'Launch-ReyondBroom.sh')
(root/'Launch-ReyondBroom.sh').chmod(0o755)
if a.sdl2:
    (root/'linux-sdl2-build').write_text('SDL2 backend\n')

audio = root/'tools/audio/ffmpeg'
audio.parent.mkdir(parents=True)
audio.write_text('#!/bin/sh\nexec /usr/bin/ffmpeg "$@"\n')
audio.chmod(0o755)
for folder in ('docs',):
    shutil.copytree(src/folder, root/folder)
for path in list(src.glob('*.md'))+list(src.glob('*LICENSE.txt')):
    shutil.copy2(path, root/path.name)
shutil.copy2(src/'docs/linux.md',root/'START-LINUX.md')
tracked = subprocess.check_output(['git','ls-files','-z'],cwd=src).decode().split('\0')
for name in filter(None,tracked):
    if name.startswith('tools/'):
        continue
    target=root/'sources/ftebroom'/name
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(src/name,target)
for tool in ('q3map2','ericw'):
    for file in (src/'tools'/tool).iterdir():
        if file.is_file() and file.suffix.lower() in ('.zip','.txt','.md','.json'):
            shutil.copy2(file,root/'tools'/tool/file.name)
for directory, revision in [('fteqw','f937b9d88f71fc4429db5fe56c6a98d922711b2e'),('imgui','4806a1924ff6181180bf5e4b8b79ab4394118875')]:
    subprocess.run(['git','-C',str(a.deps/directory),'archive','--format=zip',f'--output={root}/sources/{directory}-source.zip',revision],check=True)
subprocess.run(['git','-C',str(a.deps/'bullet3-3.25'),'archive','--format=zip',f'--output={root}/sources/bullet-source.zip','2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5','src','CMakeLists.txt','BulletConfig.cmake.in','UseBullet.cmake','bullet.pc.cmake','LICENSE.txt','VERSION'],check=True)
shutil.copy2(a.deps/'compilers/embree-v2.17.7.tar.gz',root/'tools/ericw/embree-v2.17.7-source.tar.gz')
shutil.copy2(a.deps/'compilers/embree-2.17.7/LICENSE.txt',root/'tools/ericw/LICENSE-embree.txt')
subprocess.run(['git','-C',str(a.deps/'wl-clipboard'),'archive','--format=zip',f'--output={root}/tools/clipboard/wl-clipboard-source.zip','3eb912c274042cd5deed6b478b39908a12f37498'],check=True)
shutil.copy2(a.deps/'wl-clipboard/COPYING',root/'tools/clipboard/LICENSE.txt')
(root/'build-info.txt').write_text(f'ReyondBroom {a.version}\nUbuntu 24.04 x86-64, glibc 2.39+\nBackend: {"SDL2" if a.sdl2 else "native"}\nSee docs/linux.md for dependencies and platform differences.\n')
runpy.run_path(str(src/'scripts/package-sources.py'))['separate_sources'](
    root, out, a.version, platform)
manifest={path.relative_to(root).as_posix():hashlib.sha256(path.read_bytes()).hexdigest() for path in root.rglob('*') if path.is_file()}
(root/'distribution-manifest.json').write_text(json.dumps(manifest,indent=2))
archive=out/f'ReyondBroom-{a.version}-{platform}.tar.gz'
with tarfile.open(archive,'w:gz') as f:
    f.add(root,arcname=root.name)
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_name(archive.name+'.sha256').write_text(f'{digest}  {archive.name}\n')
print(f'{archive}\n{archive.stat().st_size} bytes\nSHA256 {digest}')
