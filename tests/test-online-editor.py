# SPDX-License-Identifier: GPL-2.0-or-later
"""Run under xvfb-run: two real SDL editor instances against one local relay."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import threading
import time

src=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(src/'runtime/online'))
from server import make_server
parser=argparse.ArgumentParser();parser.add_argument('--deps',required=True);parser.add_argument('--resources',required=True);parser.add_argument('--output',required=True);parser.add_argument('--master-browser',action='store_true')
args=parser.parse_args();deps=Path(args.deps);resources=Path(args.resources).resolve();root=Path(args.output)
root.mkdir(parents=True,exist_ok=False)
server=make_server(('127.0.0.1',0),root/'relay',resources)
threading.Thread(target=server.serve_forever,daemon=True).start()
url='http://127.0.0.1:%s'%server.server_port
master=None
if args.master_browser:
    from master import Master, read_json
    master=Master(('127.0.0.1',0),allow_private=True)
    threading.Thread(target=master.serve_forever,daemon=True).start()
    master_url='http://127.0.0.1:%s'%master.server_port
    read_json(master_url+'/register',dict(port=server.server_port,key=server.relay.registration_key))
processes=[];streams=[]
try:
    for name in ('Alice','Bob'):
        install=root/name;install.mkdir();shutil.copytree(src/'runtime',install/'ftebroom')
        for file in ('default.fmf','installed.lst'):shutil.copy2(src/'runtime'/file,install/file)
        shutil.copy2(src/'csaddon.dat',install/'ftebroom/csaddon.dat')
        shutil.copy2(deps/'fteplug_ftebroom_amd64.so',install/'fteplug_ftebroom_amd64.so')
        shutil.copy2(deps/'fteqw/engine/release-sdl2/fteqw-glsdl2_64',install/'fteqw-gl64')
        downloads=install/'downloads';downloads.mkdir()
        (downloads/'librequake-v0.09').symlink_to(resources.parent.parent,target_is_directory=True)
        tools=install/'tools/ericw';tools.mkdir(parents=True)
        for tool in ('qbsp','vis','light'):
            (tools/tool).symlink_to(deps/('compilers/ericw-build/%s/%s'%(tool,tool)))
        (install/'ftebroom/ftebroom-quake1-build-options.txt').write_text('1 1 0 "" "" ""\n')
        cfg=f'''set ftebroom_skip_setup 1
exec ftebroom.cfg
set ftebroom_test_online_server "{url}"
set ftebroom_test_online_name {name}
in 5 ftebroom_test_desktop online_prepare
in 18 ftebroom_test_desktop online_join
in 32 ftebroom_test_desktop online_check
in 36 ftebroom_test_desktop online_compile
in 52 ftebroom_test_desktop online_publish
in 63 ftebroom_test_desktop online_scene_check
in 66 ftebroom_test_desktop online_preview_check
in 70 screenshot online.png
in 105 quit force
'''
        if name=='Alice':
            cfg+='''in 68 ftebroom_camera_restore 1152 -2000 1500 30 90 0 90
in 72 ftebroom_test_online add
in 73 ftebroom_save
in 76 ftebroom_test_desktop online_compile
in 90 ftebroom_test_desktop online_publish
in 98 ftebroom_test_desktop online_scene_check
in 100 ftebroom_test_desktop online_preview_check
in 102 screenshot online-after.png
'''
        if args.master_browser:
            cfg=cfg.replace('in 5 ftebroom_test_desktop online_prepare','set ftebroom_test_online_master "'+master_url+'"\nin 5 ftebroom_test_desktop online_browser_prepare').replace('in 18 ftebroom_test_desktop online_join','in 18 ftebroom_test_desktop online_browser_join')
        (install/'ftebroom/online-test.cfg').write_text(cfg)
        stream=(install/'console.txt').open('wb');streams.append(stream)
        env=dict(os.environ,SDL_VIDEODRIVER='x11',LIBGL_ALWAYS_SOFTWARE='1');env.pop('WAYLAND_DISPLAY',None)
        processes.append(subprocess.Popen([str(install/'fteqw-gl64'),'-ftebroom-child-processes','-nohome','-noupdates','-window','-condebug','-basedir',str(install),'+exec','online-test.cfg'],cwd=install,env=env,stdout=stream,stderr=subprocess.STDOUT))
    deadline=time.monotonic()+85
    bob_digest=None;bob_map_digest=None;alice_digest=None
    while time.monotonic()<deadline:
        with server.relay.lock:
            players=list(server.relay.players.values())
            if len(players)==2 and all(p['digest'] for p in players):
                bob=next(p for p in players if p['name']=='Bob')
                source=root/'relay'/str(bob['id'])/'latest.map'
                if source.exists():
                    bob_digest=bob['digest'];bob_map_digest=hashlib.sha256(source.read_bytes()).hexdigest()
                    alice_digest=next(p['digest'] for p in players if p['name']=='Alice');break
        time.sleep(.25)
    assert bob_digest,'Both first publications must complete'
    for process in processes:
        process.wait(timeout=150)
    for stream in streams:stream.close()
    for name in ('Alice','Bob'):
        log=(root/name/'console.txt').read_text(errors='replace')
        assert '_FAIL ' not in log and 'CSQC_Abort' not in log,log[-12000:]
        if args.master_browser:
            for check in ('online_server_onboarding_visible','online_master_row_received','online_master_preference_saved','online_fixed_resources_auto_prepare','online_onboarding_joined_without_profile_steps'):
                assert 'PASS '+check in log,(name,check,log[-12000:])
        for check in ('online_librequake_mounted','online_worker_started','online_plot_joined','online_profile_locked','online_compile_started','online_open_map_collision_no_vis_pipeline','online_local_bsp_complete','online_own_bsp_available','online_own_bsp_preview','inside_plot_edit','outside_plot_rolled_back','guard_undo','remote_avatar_loaded','remote_bsp_loaded'):
            if args.master_browser and check in ('online_librequake_mounted','online_worker_started'):continue
            assert 'PASS '+check in log,(name,check,log[-12000:])
        print('PASS real editor',name,root/name,flush=True)
        assert (root/name/'ftebroom/ftebroom-quake1-build-options.txt').read_text()=='1 1 0 "" "" ""\n','Ordinary compile settings changed'
    alice=next(p for p in server.relay.players.values() if p['name']=='Alice')
    bob=next(p for p in server.relay.players.values() if p['name']=='Bob')
    assert alice['revisions']==2 and bob['revisions']==1 and bob['digest']==bob_digest
    assert alice['digest']!=alice_digest,'The changed own brush must reach the newly compiled BSP'
    assert hashlib.sha256((root/'relay'/str(bob['id'])/'latest.bsp').read_bytes()).hexdigest()==bob_digest
    assert hashlib.sha256((root/'relay'/str(bob['id'])/'latest.map').read_bytes()).hexdigest()==bob_map_digest
    print('PASS recompiling Alice with Bob visible changes only Alice; Bob MAP/BSP and ordinary compile settings remain unchanged',flush=True)
finally:
    for process in processes:
        if process.poll() is None:process.terminate()
    server.shutdown();server.server_close()
    if master:master.shutdown();master.server_close()
