"""Compare borrowed-token indexing and trusted engine UV output with alpha.79."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--compiler', default='c++')
p.add_argument('--map', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
legacy = subprocess.check_output(['git', 'show', 'f5eab19:native/ftebroom_lines.h'], cwd=root).decode()
legacy = legacy[legacy.index('  std::istringstream in(bytes);'):legacy.index('  qhandle_t handle;')]
legacy = 'static std::string oldIndex(const std::string& bytes){\n' + legacy + 'return text;}\n'
token = (root / 'native/ftebroom_assets.h').read_text()
token = token[token.index('static std::string token('):token.index('static int QDECL\ncollectResource')]
cpp = r'''
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cassert>
#include "ftebroom_map_index.h"
#include "ftebroom_quake1.h"
''' + token + legacy + r'''
static void verify(const std::string& bytes){assert(oldIndex(bytes)==ftebroom::mapLineIndex(bytes));}
int main(int argc,char** argv){
 verify("// {\n{\n\"classname\" \"worldspawn\"\n\"message\" \"{ \\\" }\"\n/* {\n } */\n{\n( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) brick [ 1 0 0 0 ] [ 0 1 0 0 ] 0 1 1 0 0 0\n( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) brick [ 1 0 0 0 ] [ 0 1 0 0 ] 0 1 1 8 0 0\n}\n}\n");
 std::string plane="( 0 0 0 ) ( 0 128 0 ) ( 128 0 0 ) brick [ 1 0 0 0.125 ] [ 0 -1 0 -0.03125 ] 0 0.5 1";
 for(auto flags:{""," 0 0 0"," 134217728 3 9"})assert(ftebroom::quake1::face(plane+flags,false)==ftebroom::quake1::face(plane+flags,false,true));
 if(argc>1){std::ifstream in(argv[1],std::ios::binary);assert(in);std::string bytes((std::istreambuf_iterator<char>(in)),{});verify(bytes);
  auto start=std::chrono::steady_clock::now();auto old=oldIndex(bytes);auto middle=std::chrono::steady_clock::now();auto fast=ftebroom::mapLineIndex(bytes);auto end=std::chrono::steady_clock::now();assert(old==fast);
  std::cout<<"Index "<<bytes.size()<<" bytes: old_ms="<<std::chrono::duration<double,std::milli>(middle-start).count()<<" new_ms="<<std::chrono::duration<double,std::milli>(end-middle).count()<<"\n";
  std::istringstream rows(bytes);std::string line;size_t faces=0;while(std::getline(rows,line)){auto first=line.find_first_not_of(" \t\r");if(first==line.npos||line[first]!='('||line.find('[')==line.npos)continue;
    line=line.substr(first);assert(ftebroom::quake1::face(line,false)==ftebroom::quake1::face(line,false,true));++faces;}
  std::cout<<"Identical UV output: "<<faces<<" faces\n";
 }
 std::cout<<"PASS map indexing, mixed contents, comments/strings and exact Valve 220 output\n";
}
'''
with tempfile.TemporaryDirectory() as folder:
    source, exe = Path(folder) / 'test.cpp', Path(folder) / 'test.exe'
    source.write_text(cpp)
    subprocess.run([a.compiler, '-std=c++17', '-O2', '-I'+str(root/'native'), str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)] + ([str(a.map)] if a.map else []), check=True)
