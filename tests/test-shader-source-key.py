"""Mixed-case WAD names must find the same shader-source bucket."""
import argparse
from pathlib import Path
import subprocess
import tempfile
p = argparse.ArgumentParser()
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'gl/gl_shader.c').read_text()
start = s.index('static unsigned int Shader_SourceKey(')
helper = s[start:s.index('static shadercache_t **shader_hash;', start)]
assert s.count('= Shader_SourceKey (') == 3
checks = r"""
int main(void) {
 const char *names[]={"TechTrim64_b","textures/TechTrim64_b","TEXTURES/TECHTRIM64_B","textures/techtrim64_b"};
 for(unsigned modulus=1;modulus<10000;modulus+=37){
  assert(Shader_SourceKey(names[1],modulus)==Shader_SourceKey(names[2],modulus));
  assert(Shader_SourceKey(names[1],modulus)==Shader_SourceKey(names[3],modulus));
  assert(Shader_SourceKey(names[0],modulus)<modulus);
 }
 puts("PASS: shader source insertion, lookup and inspection use one case-insensitive key");
}
"""
with tempfile.TemporaryDirectory() as directory:
 source=Path(directory)/'test.c'
 exe=Path(directory)/'test.exe'
 source.write_text('#include <assert.h>\n#include <stdio.h>\n'+helper+checks)
 subprocess.run([a.compiler,'-std=c99',str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
