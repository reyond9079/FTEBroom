"""Reproducible large MAP benchmark; all files live in a disposable output root."""
import argparse,os,runpy,shutil,subprocess,sys,re,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--engine',type=Path,required=True);p.add_argument('--host',type=Path,required=True);p.add_argument('--qc',type=Path,default=Path('csaddon.dat'));p.add_argument('--brushes',type=int,default=20000)
p.add_argument('--verify',action='store_true')
a=p.parse_args();src=Path(__file__).resolve().parents[1];root=a.output.resolve();root.mkdir(exist_ok=False,parents=True);os.chdir(src)
sys.argv=['make-quake1-fixture.py',str(root)];runpy.run_path('tests/make-quake1-fixture.py',run_name='__main__')
editor=root/'editor';shutil.copytree(src/'runtime',editor/'ftebroom',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
for name in ('default.fmf','installed.lst'):shutil.copy2(src/'runtime'/name,editor/name)
shutil.copy2(a.qc,editor/'ftebroom/csaddon.dat');engine=editor/('fteqw64.exe' if os.name=='nt' else 'fteqw-gl64');shutil.copy2(a.engine,engine)
shutil.copy2(a.host,editor/('fteplug_ftebroom_x64.dll' if os.name=='nt' else 'fteplug_ftebroom_amd64.so'))
if os.name=='nt':
 for source,target in [('zlib1.dll','zlib1.dll'),('libpng16-16.dll','libpng16.dll')]:shutil.copy2(Path('C:/msys64/mingw64/bin')/source,editor/target)
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0;options=dict(startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
else:options={}
def box(x,y,z):
 X,Y,Z=x+64,y+64,z+64
 planes=[[(x,Y,Z),(x,y,Z),(x,y,z)],[(X,y,z),(X,y,Z),(X,Y,Z)],[(X,y,Z),(X,y,z),(x,y,z)],[(x,Y,z),(X,Y,z),(X,Y,Z)],[(X,Y,z),(x,Y,z),(x,y,z)],[(x,y,Z),(x,Y,Z),(X,Y,Z)]]
 return '{\n'+''.join(' '.join('( %s )'%(' '.join(map(str,q))) for q in plane)+' q1test 0 0 0 1 1\n' for plane in planes)+'}\n'
text='// Game: Quake\n// Format: Standard\n{\n"classname" "worldspawn"\n'+''.join(box((n%200)*128,(n//200)*128,0) for n in range(a.brushes))+'}\n{\n"classname" "info_player_start"\n"origin" "64 -128 96"\n}\n'
(editor/'ftebroom/maps/large.map').write_text(text,encoding='ascii')
cfg='''exec ftebroom.cfg
set ftebroom_skip_setup 1
set vid_vsync 0
set cl_maxfps 0
set ftebroom_debug_lines 1
in 3 mapedit maps/large.map
in 9 exec large-phases.cfg
'''
(editor/'ftebroom/large-phases.cfg').write_text(f'set ftebroom_test_large_count {a.brushes}\n'+'''ftebroom
ca_show 1
ftebroom_camera_restore 512 -512 768 25 70 0 90
in 1 set ftebroom_benchmark 1
in 11 set ftebroom_right 1
in 11 set ftebroom_benchmark 1
in 20 set ftebroom_right 0
in 20 ftebroom_camera_restore 512 -512 768 25 70 0 90
in 21 screenshot large.png
'''+('in 22 ftebroom_test_generators large74\n' if a.verify else '')+'in 25 quit force\n')
(editor/'ftebroom/large-test.cfg').write_text(cfg)
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',SDL_VIDEODRIVER='x11');env.pop('WAYLAND_DISPLAY',None)
with (editor/'console.txt').open('wb') as out:
 process=subprocess.Popen([str(engine),'-ftebroom-child-processes','-nohome','-noupdates','-window','-width','1280','-height','800','-condebug','-basedir',str(editor),'+exec','large-test.cfg'],cwd=editor,env=env,stdout=out,stderr=subprocess.STDOUT,**options)
 try:process.wait(timeout=300)
 except Exception:process.terminate();raise
log=(editor/('ftebroom/qconsole.log' if os.name=='nt' else 'console.txt')).read_text(errors='replace');assert process.returncode==0 and 'CSQC_Abort' not in log and 'runaway loop' not in log,log[-5000:]
if a.verify:
 assert '_FAIL ' not in log,log[-6000:]
 for name in ('loaded_brush_count','linear_source_line_index','typed_node_lists','exact_pick','selection_cache_invalidation','edited_id_lookup','undo_lookup','deselection_cache_invalidation'):assert 'PASS large74_'+name in log,(name,log[-6000:])
lines=[line for line in log.splitlines() if re.search(r'FTEBROOM_(BENCHMARK|FRAME_PROFILE|PROFILE|VISUAL_PROFILE)',line)]
print('\n'.join(lines));assert len([line for line in lines if 'FTEBROOM_BENCHMARK' in line])==2,log[-5000:]
(root/'results.json').write_text(json.dumps({'brushes':a.brushes,'lines':lines},indent=2));print(root)
