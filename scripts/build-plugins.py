"""Build the bundled SDK example independently of the editor and engine."""
from pathlib import Path
import argparse, os, shutil, subprocess
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--compiler',default='g++')
a=p.parse_args();src=Path(__file__).resolve().parents[1]
folder=a.output/'rubble';folder.mkdir(parents=True,exist_ok=True)
windows=os.name=='nt';binary=folder/('rubble.dll' if windows else 'rubble.so')
args=[a.compiler,'-std=c++17','-O2','-shared', '-static' if windows else '-fPIC',
      '-I'+str(src/'sdk'),str(src/'plugins/rubble/rubble.cpp'),'-o',str(binary)]
subprocess.run(args,check=True)
shutil.copy2(src/'plugins/rubble/plugin.ini',folder/'plugin.ini')
print(binary)
