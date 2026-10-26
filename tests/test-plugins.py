"""Exercise a separately compiled plugin through the real editor and Bullet."""
import argparse, os, runpy, shutil, subprocess, sys
from pathlib import Path
src=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--deps',type=Path,required=True);p.add_argument('--windows-plugin',type=Path);p.add_argument('--native',action='store_true');p.add_argument('--horizontal',action='store_true')
a=p.parse_args();root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);deps=a.deps.resolve();os.chdir(src)
sys.argv=['make-quake1-fixture.py',str(root)];runpy.run_path('tests/make-quake1-fixture.py',run_name='__main__')
editor=root/'editor';game=root/'id1';shutil.copytree(src/'runtime',editor/'ftebroom',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
for name in ('default.fmf','installed.lst'):shutil.copy2(src/'runtime'/name,editor/name)
shutil.copy2(src/'csaddon.dat',editor/'ftebroom/csaddon.dat')
if os.name=='nt':
    engine=editor/'fteqw64.exe';shutil.copy2(deps/'engine/release-ftebroom/fteglqw64.exe',engine);shutil.copy2(a.windows_plugin,editor/'fteplug_ftebroom_x64.dll')
    shutil.copytree(a.windows_plugin.resolve().parent/'plugins',editor/'plugins')
    for source,target in [('zlib1.dll','zlib1.dll'),('libpng16-16.dll','libpng16.dll')]:shutil.copy2(Path('C:/msys64/mingw64/bin')/source,editor/target)
    startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0;options=dict(startupinfo=startup,creationflags=subprocess.CREATE_NO_WINDOW)
else:
    engine=editor/'fteqw-gl64';shutil.copy2(deps/('fteqw/engine/release-linux/fteqw-gl64' if a.native else 'fteqw/engine/release-sdl2/fteqw-glsdl2_64'),engine)
    shutil.copy2(deps/'fteplug_ftebroom_amd64.so',editor/'fteplug_ftebroom_amd64.so');shutil.copytree(deps/'plugins',editor/'plugins');options={}
cfg=f'''exec ftebroom.cfg
set ftebroom_skip_setup 1
set ftebroom_test_rubble_horizontal {int(a.horizontal)}
set ftebroom_test_import_root "{game.as_posix()}"
in 3 ftebroom_test_desktop external69load
in 5 ftebroom_test_desktop plugin72open
in 6 ftebroom_test_generators rubble72setup
in 8 ftebroom_test_desktop plugin72apply
in 12 screenshot plugin-working.png
in 14 ftebroom_test_desktop plugin72done
in 15 ftebroom_test_generators rubble72check
in 16 screenshot plugin-result.png
in 17 ftebroom_test_generators rubble72invalid
in 18 ftebroom_test_generators rubble73guard
in 21 quit force
'''
(editor/'ftebroom/plugins-test.cfg').write_text(cfg)
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',SDL_VIDEODRIVER='x11');env.pop('WAYLAND_DISPLAY',None)
with (editor/'console.txt').open('wb') as out:
    process=subprocess.Popen([str(engine),'-ftebroom-child-processes','-nohome','-noupdates','-window','-width','1280','-height','800','-condebug','-basedir',str(editor),'+exec','plugins-test.cfg'],cwd=editor,env=env,stdout=out,stderr=subprocess.STDOUT,**options)
    try:process.wait(timeout=45)
    except Exception:process.terminate();raise
log=(editor/('ftebroom/qconsole.log' if os.name=='nt' else 'console.txt')).read_text(errors='replace')
assert '_FAIL ' not in log and 'CSQC_Abort' not in log,log[-16000:]
for name in ('plugin72_external_module_loaded','plugin72_sdk_button_drawn','plugin72_async_workflow_complete','plugin72_unload','plugin72_reload','rubble72_pipeline_complete','rubble72_fragments_fell','rubble72_undo_settle','rubble72_undo_geometry','rubble72_redo_pipeline','rubble72_invalid_request_atomic'):
    assert 'PASS '+name in log,(name,log[-16000:])
for name in ('rubble73_fragments_in_render_tree','rubble73_grouped_plug_entities','rubble73_drop_diagnostics','rubble73_no_flying_result','rubble73_stale_selection_rejected','rubble73_layout_retries_atomic','rubble73_certified_layout_real_geometry','rubble73_certified_layout_undo')+ (('rubble73_horizontal_downward_release',) if a.horizontal else ()):
    assert 'PASS '+name in log,(name,log[-16000:])
print('PASS independently loaded plugin: SDK UI, holes/fragments, Bullet fall, selection, Undo/Redo, invalid-input rollback, unload/reload')
