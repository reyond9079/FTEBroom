import importlib.util
import io
import json
import hashlib
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import time
import unittest
from unittest.mock import patch

script=Path(__file__).resolve().parents[1]/'runtime/update-ftebroom.py'
spec=importlib.util.spec_from_file_location('updater',script)
updater=importlib.util.module_from_spec(spec);spec.loader.exec_module(updater)

class UpdateTests(unittest.TestCase):
    def test_cleanup_keeps_latest_backup_and_user_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);updates=root/'updates';updates.mkdir()
            keep=updates/'backup-new';keep.mkdir();(keep/'engine').write_text('rollback')
            for name in ('backup-old','stage-old'):
                (updates/name).mkdir();(updates/name/'file').write_text('cache')
            archive=updates/'ReyondBroom-0.5.0-alpha.66-Linux-x64.tar.gz';archive.write_text('cache')
            (updates/'notes.txt').write_text('user')
            outside=root/'outside';outside.mkdir();(outside/'map').write_text('user')
            (updates/'stage-link').symlink_to(outside, target_is_directory=True)
            updater.cleanup_updates(updates,keep)
            self.assertTrue((keep/'engine').exists())
            self.assertTrue((outside/'map').exists())
            self.assertTrue((updates/'notes.txt').exists())
            self.assertFalse(archive.exists())
            self.assertFalse((updates/'stage-old').exists())
            self.assertFalse((updates/'backup-old').exists())

    def test_platform_selection(self):
        def release(number,platform):
            name=f'ReyondBroom-0.5.0-alpha.{number}-{platform}-x64.'+('tar.gz' if platform=='Linux' else 'zip')
            return dict(tag_name=f'v0.5.0-alpha.{number}',assets=[dict(name=name),dict(name=name+'.sha256')])
        result=updater.select_release([release(42,'Windows'),release(41,'Linux')],'0.5.0-alpha.40')
        self.assertEqual(result[0],'v0.5.0-alpha.41')
        self.assertIsNone(updater.select_release([release(42,'Windows')],'0.5.0-alpha.40'))

    def test_sdl_selection_does_not_switch_backend(self):
        name='ReyondBroom-0.5.0-alpha.70-Linux-SDL2-x64.tar.gz'
        sdl=dict(tag_name='v0.5.0-alpha.70',assets=[dict(name=name),dict(name=name+'.sha256')])
        self.assertEqual(updater.select_release([sdl],'0.5.0-alpha.69',True)[1]['name'],name)
        self.assertIsNone(updater.select_release([sdl],'0.5.0-alpha.69'))

    def make_package(self,folder):
        files={'fteqw-gl64':b'new engine','fteplug_ftebroom_amd64.so':b'plugin','Launch-ReyondBroom.sh':b'launcher',
               'ftebroom/appearance.cfg':b'0 0 0 0','ftebroom/ftebroom-quake1.txt':b'new defaults',
               'ftebroom/maps/custom.map':b'package map','ftebroom/custom.cfg':b'factory'}
        files['distribution-manifest.json']=json.dumps({k:hashlib.sha256(v).hexdigest() for k,v in files.items()}).encode()
        path=folder/'package.tar.gz'
        with tarfile.open(path,'w:gz') as tar:
            for name,data in files.items():
                info=tarfile.TarInfo('ReyondBroom-Linux-x64/'+name);info.size=len(data);info.mode=0o755 if name.endswith('.sh') else 0o644
                tar.addfile(info,io.BytesIO(data))
        return path

    def test_install_wait_and_preservation(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'fteqw-gl64').write_text('old engine')
            (root/'ftebroom/maps').mkdir(parents=True)
            custom={'appearance.cfg':'4 0 2 1','ftebroom-quake1.txt':'my mod','maps/custom.map':'my geometry','custom.cfg':'my binding'}
            for name,value in custom.items():(root/'ftebroom'/name).write_text(value)
            package=self.make_package(root)
            parent=subprocess.Popen(['/bin/sleep','3'])
            worker=subprocess.Popen([sys.executable,str(script),'--root',str(root),'--current-version','0.5.0-alpha.40','--parent-id',str(parent.pid),'--local-package',str(package)])
            try:
                time.sleep(1)
                self.assertEqual((root/'fteqw-gl64').read_text(),'old engine')
                parent.wait();self.assertEqual(worker.wait(timeout=15),0)
                self.assertEqual((root/'fteqw-gl64').read_text(),'new engine')
                for name,value in custom.items():self.assertEqual((root/'ftebroom'/name).read_text(),value)
                self.assertTrue((root/'Launch-ReyondBroom.sh').stat().st_mode&0o111)
                self.assertTrue(list((root/'updates').glob('backup-*/fteqw-gl64')))
            finally:
                if parent.poll() is None:parent.kill();parent.wait()
                if worker.poll() is None:worker.kill();worker.wait()

    def test_reject_links_and_traversal(self):
        for name,kind in [('ReyondBroom-Linux-x64/../escape',tarfile.REGTYPE),('ReyondBroom-Linux-x64/link',tarfile.SYMTYPE)]:
            with self.subTest(name=name),tempfile.TemporaryDirectory() as temp:
                root=Path(temp);archive=root/'bad.tar.gz';stage=root/'stage';stage.mkdir()
                with tarfile.open(archive,'w:gz') as tar:
                    info=tarfile.TarInfo(name);info.type=kind;info.linkname='/tmp/outside';tar.addfile(info)
                with self.assertRaises(ValueError):updater.stage_package(archive,stage)

    def test_rollback_on_install_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)/'root';stage=Path(temp)/'stage';backup=Path(temp)/'backup'
            for p in (root,stage,backup):p.mkdir()
            for name in ('a','b'):(root/name).write_text('old');(stage/name).write_text('new')
            original=updater.os.replace
            def replace(source,target):
                if Path(target).name=='b' and str(source).endswith('.tmp'):raise OSError('simulated failure')
                return original(source,target)
            with patch.object(updater.os,'replace',replace):
                with self.assertRaises(OSError):updater.install(root,stage,backup)
            self.assertEqual((root/'a').read_text(),'old');self.assertEqual((root/'b').read_text(),'old')

unittest.main()
