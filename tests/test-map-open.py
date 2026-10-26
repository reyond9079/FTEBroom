"""Open an external, unmodified MAP with missing WADs in a disposable install."""
import argparse, hashlib, json, os, re, runpy, shutil, subprocess, sys
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--engine',type=Path,required=True)
p.add_argument('--host',type=Path,required=True)
p.add_argument('--map',type=Path,required=True)
a=p.parse_args();src=Path(__file__).resolve().parents[1];os.chdir(src)
data=a.map.resolve().read_bytes();digest=hashlib.sha256(data).hexdigest()
source=data.decode('utf8');brushes=len(re.findall(r'^// brush ',source,re.M));entities=len(re.findall(r'^"classname" ',source,re.M))
assert brushes and entities
root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
sys.argv=['make-quake1-fixture.py',str(root)];runpy.run_path('tests/make-quake1-fixture.py',run_name='__main__')
editor=root/'editor';game=root/'id1';shutil.copytree(src/'runtime',editor/'ftebroom',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
for name in ('default.fmf','installed.lst'):shutil.copy2(src/'runtime'/name,editor/name)
shutil.copy2(src/'csaddon.dat',editor/'ftebroom/csaddon.dat')
engine=editor/('fteqw64.exe' if os.name=='nt' else 'fteqw-gl64');shutil.copy2(a.engine,engine)
shutil.copy2(a.host,editor/('fteplug_ftebroom_x64.dll' if os.name=='nt' else 'fteplug_ftebroom_amd64.so'))
if os.name=='nt':
 for source,target in [('zlib1.dll','zlib1.dll'),('libpng16-16.dll','libpng16.dll')]:shutil.copy2(Path('C:/msys64/mingw64/bin')/source,editor/target)
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0;options=dict(startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
else:options={}
# Include an obsolete configured WAD and one usable external WAD simultaneously.
wads=(game/'absent.wad').as_posix()+'\n'+(game/'test textures.wad').as_posix()
(editor/'ftebroom-project.txt').write_text(game.as_posix())
(editor/'ftebroom-game-profile.txt').write_text('2 0 "" "" "" "" 1 0 ""\n')
# std::quoted preserves actual newlines; JSON's \n would become a literal n.
(editor/'ftebroom-quake1.txt').write_text('"" "'+game.as_posix()+'" "'+wads+'" "" 1 0 "" 0\n')
commands=['set ftebroom_skip_setup 1','exec ftebroom.cfg','set vid_vsync 0','set cl_maxfps 0','set cl_idlefps 0','set cl_yieldcpu 0',
 'in 6 set ftebroom_grid_visible 1','in 6 set ftebroom_grid_snap 1',
 'in 7 ftebroom_test_key kp_ins','in 8 ftebroom_test_grid flags 0 1 numpad75_zero',
 'in 9 ftebroom_test_key Alt+kp_ins','in 10 ftebroom_test_grid flags 0 0 numpad75_alt_zero',
 'in 11 ftebroom_test_key kp_ins text','in 12 ftebroom_test_grid flags 0 0 numpad75_text_guard',
 'in 13 ftebroom_test_key kp_ins','in 14 ftebroom_test_grid flags 1 0 numpad75_zero_again',
 f'in 15 ftebroom_test_filepath open "{a.map.resolve().as_posix()}"',
 f'set ftebroom_test_map_brushes {brushes}',f'set ftebroom_test_map_entities {entities}',
 'in 48 ftebroom_test_generators map75','in 49 ftebroom_test_desktop map75check',
 'in 50 set ftebroom_benchmark 1','in 60 screenshot map-open.png',
 'in 61 set ftebroom_right 1','in 61 set ftebroom_benchmark 1','in 72 set ftebroom_right 0',
 'in 73 ftebroom_test_generators labels75','in 73 ftebroom_test_model_registry','in 74 quit force']
(editor/'ftebroom/map-open.cfg').write_text('\n'.join(commands)+'\n',encoding='utf8')
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',SDL_VIDEODRIVER='x11');env.pop('WAYLAND_DISPLAY',None)
with (editor/'console.txt').open('wb') as out:
 process=subprocess.Popen([str(engine),'-ftebroom-child-processes','-nohome','-noupdates','-window','-width','1280','-height','800','-condebug','-basedir',str(editor),'+exec','map-open.cfg'],cwd=editor,env=env,stdout=out,stderr=subprocess.STDOUT,**options)
 try:process.wait(timeout=240)
 except Exception:process.terminate();raise
log=(editor/('ftebroom/qconsole.log' if os.name=='nt' else 'console.txt')).read_text(errors='replace')
assert process.returncode==0 and not re.search(r'_FAIL |CSQC_Abort|runaway loop|MAX_MOD_KNOWN',log),log[-8000:]
for marker in ('numpad75_zero','numpad75_alt_zero','numpad75_text_guard','numpad75_zero_again','map75_all_brushes_loaded','map75_all_entities_loaded','map75_cache_warmup_finished','map75_resources_ready','map75_external_document_open','map75_valid_wad_survives_missing_archives','map75_missing_wads_reported','labels75_matches_full_traces','labels75_native_path_exercised'):
 assert 'PASS '+marker in log,(marker,log[-8000:])
assert 'FTEBROOM_REGISTRY_PASS' in log
assert hashlib.sha256(a.map.resolve().read_bytes()).hexdigest()==digest,'Input MAP changed'
print('\n'.join(line for line in log.splitlines() if 'PASS ' in line or 'FTEBROOM_BENCHMARK' in line));print('PASS: original MAP unchanged:',digest);print(root)
