# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual editor render, hierarchy, gizmo and exported-model smoke test."""
import argparse
import hashlib
import os
from pathlib import Path
import runpy
import shutil
import struct
import subprocess
import sys
from PIL import Image

src=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--output',required=True)
parser.add_argument('--deps',required=True)
parser.add_argument('--windows-plugin')
parser.add_argument('--mod',help='Use an actual Quake 1 mod archive set in the disposable fixture')
args=parser.parse_args();root=Path(args.output).resolve();root.mkdir(parents=True,exist_ok=False);deps=Path(args.deps).resolve()
old_args=sys.argv;sys.argv=['make-quake1-fixture.py',str(root)];os.chdir(src)
runpy.run_path('tests/make-quake1-fixture.py',run_name='__main__');sys.argv=old_args
editor=root/'editor';game=root/'id1';models=game/'models/qa';models.mkdir(parents=True)
if args.mod:
    for pak in game.glob('*.pak'):pak.unlink()
    for pak in Path(args.mod).glob('*.pak'):shutil.copy2(pak,game/pak.name)
def tga(rgb):return bytes([0,0,2])+bytes(9)+struct.pack('<HHBB',2,2,24,32)+bytes(rgb[::-1])*4
(models/'red.tga').write_bytes(tga((240,20,30)));(models/'green.tga').write_bytes(tga((20,240,50)))
def md3(view=False):
    def surface(index):
        nv=3;nt=1;frames=2;end=212+nv*8*frames
        data=bytearray(end);struct.pack_into('<4s64s10i',data,0,b'IDP3',('mesh'+str(index)).encode(),0,frames,1,nv,nt,108,120,188,212,end)
        struct.pack_into('<3i',data,108,0,1,2);struct.pack_into('<64si',data,120,('models/qa/'+('red' if index==0 else 'green')).encode(),0)
        struct.pack_into('<6f',data,188,0,0,1,0,0,1)
        points=[(-30+index*40,0,0),(-5+index*40,0,0),(-30+index*40,0,45)] if not view else [(25,-10+index*20,-12),(40,-10+index*20,-12),(25,5+index*20,-4)]
        for frame in range(frames):
            for i,(x,y,z) in enumerate(points):struct.pack_into('<3hH',data,212+frame*nv*8+i*8,int(x*64),int(y*64),int((z+frame*2)*64),0)
        return data
    bodies=surface(0)+surface(1);frame=struct.pack('<10f16s',-32,-16,-16,48,32,50,0,0,0,80,b'pose')
    return struct.pack('<4si64s9i',b'IDP3',15,b'editor fixture',0,2,0,2,0,108,220,220,220+len(bodies))+frame*2+bodies
(models/'test.md3').write_bytes(md3());(models/'v_test.md3').write_bytes(md3(True))
runpy.run_path(str(src/'tests/make-model33.py'))['write_model'](models/'bones.iqm')
before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in models.iterdir()}
shutil.copytree(src/'runtime',editor/'ftebroom',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
for name in ('default.fmf','installed.lst'):shutil.copy2(src/'runtime'/name,editor/name)
shutil.copy2(src/'csaddon.dat',editor/'ftebroom/csaddon.dat')
if os.name=='nt':
    engine=editor/'fteqw64.exe';shutil.copy2(deps/'engine/release-ftebroom/fteglqw64.exe',engine)
    shutil.copy2(args.windows_plugin,editor/'fteplug_ftebroom_x64.dll')
    for name in ('zlib1.dll','libpng16-16.dll'):
        if Path('C:/msys64/mingw64/bin',name).exists():shutil.copy2(Path('C:/msys64/mingw64/bin',name),editor/name)
    shutil.copy2('C:/msys64/mingw64/bin/libpng16-16.dll',editor/'libpng16.dll')
    startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    options=dict(startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
else:
    engine=editor/'fteqw-gl64';shutil.copy2(deps/'fteqw/engine/release-sdl2/fteqw-glsdl2_64',engine)
    shutil.copy2(deps/'fteplug_ftebroom_amd64.so',editor/'fteplug_ftebroom_amd64.so');options={}
cfg=f'''exec ftebroom.cfg
set ftebroom_skip_setup 1
set ftebroom_test_import_root "{game.as_posix()}"
in 5 ftebroom_test_desktop external69load
in 7 ftebroom_test_entities player72
in 8 ftebroom_test_desktop medit71open
in 11 ftebroom_test_desktop medit71check
in 12 ftebroom_test_desktop medit71pixels
in 13 screenshot editor-textured.png
in 14 ftebroom_test_desktop medit71wire
in 15 screenshot editor-wire.png
in 16 ftebroom_test_desktop medit71solid
in 17 screenshot editor-solid.png
in 18 ftebroom_test_desktop medit71export
in 22 ftebroom_test_desktop medit71exports
in 24 screenshot editor-bones.png
in 25 ftebroom_test_desktop medit71bones
in 27 screenshot editor-player.png
in 28 ftebroom_test_desktop medit71player
in 30 ftebroom_test_desktop medit71drag
in 32 screenshot editor-gizmo.png
in 34 ftebroom_test_desktop medit71mdl
in 37 ftebroom_test_desktop medit71mdlwrite
in 41 ftebroom_test_desktop medit71mdlcheck
in 43 screenshot editor-mdl.png
in 45 ftebroom_test_desktop medit71reloadobj
in 48 ftebroom_test_desktop medit71reloadcheck
in 49 ftebroom_test_desktop medit71reloadiqm
in 52 ftebroom_test_desktop medit71reloadcheck
in 53 ftebroom_test_desktop medit71reloadmd5
in 56 ftebroom_test_desktop medit71reloadcheck
in 58 ftebroom_test_desktop medit72camera
in 61 ftebroom_test_desktop medit72cameraStart
in 62 ftebroom_test_desktop medit72cameraEnd
in 63 ftebroom_test_desktop medit72hoverStart
in 64 ftebroom_test_desktop medit72hoverEnd
in 65 screenshot editor-floor-camera.png
in 66 quit force
'''
(editor/'ftebroom/editor-test.cfg').write_text(cfg)
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',SDL_VIDEODRIVER='x11');env.pop('WAYLAND_DISPLAY',None)
with (editor/'console.txt').open('wb') as out:
    process=subprocess.Popen([str(engine),'-ftebroom-child-processes','-nohome','-noupdates','-window','-width','1280','-height','800','-condebug','-basedir',str(editor),'+exec','editor-test.cfg'],cwd=editor,env=env,stdout=out,stderr=subprocess.STDOUT,**options)
    try:process.wait(timeout=85)
    except Exception:process.terminate();raise
log=(editor/('ftebroom/qconsole.log' if os.name=='nt' else 'console.txt')).read_text(errors='replace')
assert '_FAIL ' not in log and 'CSQC_Abort' not in log,log[-14000:]
for key in ('medit71_separate_editor','medit71_mesh_hierarchy','medit71_parent_propagates','medit71_preview_ready','medit71_actual_surfaces_submitted','medit71_md3_export','medit71_all_export_files','medit71_bones_overlay','medit71_viewmodel_auto_camera','medit71_player_camera_origin','medit71_mouse_axis_move','medit71_mdl_original_skins_uv_frames_retained','medit72_rmb_fly_capture','medit72_wasd_camera_moves','medit72_map_camera_unchanged','medit72_cursor_release','medit72_bounded_floor_grid_and_axes','medit72_sky_overlays_default_off','medit72_hover_wasd_without_capture','medit72_hover_camera_moves','medit72_camera_does_not_edit_model'):
    assert 'PASS '+key in log,(key,log[-14000:])
assert 'PASS player72_loaded_after_resource_mount' in log and 'PASS player72_recovers_without_command' in log,log[-14000:]
for name,digest in before.items():assert hashlib.sha256((models/name).read_bytes()).hexdigest()==digest,name
for image in ('editor-textured.png','editor-wire.png','editor-solid.png','editor-bones.png','editor-player.png','editor-gizmo.png'):assert (editor/'ftebroom'/image).is_file(),image
assert log.count('PASS medit71_export_reopened_in_fte')==3,log[-8000:]
x,y,w,h=map(float,(editor/'ftebroom/data/model-editor-rect.txt').read_text().split())
image=Image.open(editor/'ftebroom/editor-textured.png').convert('RGB').crop((int(x),int(y),int(x+w),int(y+h)))
red=sum(r>150 and g<100 and b<100 for r,g,b in image.getdata())
green=sum(g>150 and r<100 and b<100 for r,g,b in image.getdata())
assert red>500 and green>500,('Textured model is not visible',red,green)
print('PASS rendered diffuse textures:',red,'red pixels and',green,'green pixels')
print('PASS actual editor: hierarchy, transforms, rendered modes, skeletal animation/bones, viewmodel camera, mouse-axis movement, five exports; source hashes unchanged',flush=True)
restart='''exec ftebroom.cfg
set ftebroom_skip_setup 1
in 6 ftebroom_camera_restore -40 -200 110 5 90 0 90
in 7 ftebroom_test_entities player72
in 9 screenshot player-after-restart.png
in 10 quit force
'''
(editor/'ftebroom/restart-test.cfg').write_text(restart)
old_log=(editor/'ftebroom/qconsole.log').read_text(errors='replace') if os.name=='nt' else ''
with (editor/'restart-console.txt').open('wb') as out:
    process=subprocess.Popen([str(engine),'-ftebroom-child-processes','-nohome','-noupdates','-window','-width','1280','-height','800','-condebug','-basedir',str(editor),'+exec','restart-test.cfg'],cwd=editor,env=env,stdout=out,stderr=subprocess.STDOUT,**options)
    try:process.wait(timeout=30)
    except Exception:process.terminate();raise
restart_log=(editor/('ftebroom/qconsole.log' if os.name=='nt' else 'restart-console.txt')).read_text(errors='replace')
if old_log and restart_log.startswith(old_log):restart_log=restart_log[len(old_log):]
assert 'PASS player72_loaded_after_resource_mount' in restart_log and 'PASS player72_recovers_without_command' in restart_log,restart_log[-12000:]
assert 'Unknown command "ftebroom_reload_asset_models"' not in restart_log and 'Unknown command "ftebroom_entity_definitions_loaded"' not in restart_log
print('PASS Quake 1 player model after process restart with saved mod resources',flush=True)
