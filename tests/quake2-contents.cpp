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
 for(auto tail:{" 1 0 0"," 2 16 0"," 8 0 0"," 9 0 0"," 11 0 0"," -1 0 0",""}){auto input=plane+axes+tail;assert(fix(input)==input&&n==0);}
 // An integer texture rotation is not an extended contents field.
 auto input=plane+"0 0 3 1 1\n";assert(fix(input)==input&&n==0);
 assert(fix("\"message\" \"3 16 0\"\n")=="\"message\" \"3 16 0\"\n"&&n==0);
 std::cout<<"PASS: Q2 Valve/classic contents repair, detail bits, flags, no-extra fields and unrelated conflicts preserved\n";
}
