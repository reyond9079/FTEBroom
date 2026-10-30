// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_quake2_contents.h"
#include <cassert>
#include <iostream>
int main(){
 const std::string plane="( 0 0 0 ) ( 0 1 0 ) ( 0 0 1 ) \"glass\" ";
 const std::string axes="[ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1";
 size_t n;auto fix=[&](const std::string& s){return ftebroom::q2CompileContents(s,n);};
 assert(fix(plane+axes+" 3 16 0\n")==plane+axes+" 2 16 0\n"&&n==1);
 assert(fix(plane+"0 0 0 1 1 134217731 32 100 // detail glass\n")==plane+"0 0 0 1 1 134217730 32 100 // detail glass\n"&&n==1);
 for(int kind:{2,4,8,16,32,64}){
  assert(fix(plane+axes+" "+std::to_string(kind|1)+" 0 0\n")==plane+axes+" "+std::to_string(kind)+" 0 0\n"&&n==1);
  assert(fix(plane+"0 0 0 1 1 "+std::to_string((kind|1)|134217728)+" 32 100\n")==plane+"0 0 0 1 1 "+std::to_string(kind|134217728)+" 32 100\n"&&n==1);
 }
 for(auto tail:{" 0 0 0"," 1 0 0"," 2 16 0"," 4 0 0"," 8 0 0"," 16 0 0"," 32 0 0"," 64 0 0"," 6 0 0"," 11 0 0"," 25 0 0"," 49 0 0"," -1 0 0",""}){auto input=plane+axes+tail;assert(fix(input)==input&&n==0);}
 // An integer texture rotation is not an extended contents field.
 auto input=plane+"0 0 3 1 1\n";assert(fix(input)==input&&n==0);
 assert(fix("\"message\" \"3 16 0\"\n")=="\"message\" \"3 16 0\"\n"&&n==0);
 std::cout<<"PASS: Q2 WINDOW/AUX/LAVA/SLIME/WATER/MIST legacy repair; Valve/classic syntax, detail bits and surface flags preserved; ambiguous mixed contents left unchanged\n";
}
