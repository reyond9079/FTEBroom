// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_leak_path.h"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace ftebroom::leak;
int main(){
 auto points=parse("1 2 3\n1 2 3\n(4 5 6)\n-7.5 8e2 0\n");assert(points.size()==3&&points[1][2]==6);
 for(auto text:{"","nan 0 0","inf 0 0","10000001 0 0","1 2","1 2 3 junk","(1 2 3]"}){bool rejected=false;try{parse(text);}catch(...){rejected=true;}assert(rejected);}
 auto root=std::filesystem::temp_directory_path()/("reyond-leak-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);
 auto bsp=root/"snapshot.bsp";std::ofstream(root/"snapshot.lin")<<"0 1 2\n";assert(find(bsp).extension()==".lin");std::ofstream(root/"snapshot.pts")<<"3 4 5\n";assert(find(bsp).extension()==".pts");std::ofstream(root/"snapshot.pts",std::ios::trunc);assert(find(bsp).extension()==".lin");std::filesystem::remove_all(root);
 std::cout<<"PASS: PTS/LIN detection, duplicate removal, coordinates, invalid and oversized-coordinate rejection\n";
}
