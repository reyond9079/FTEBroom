// SPDX-License-Identifier: GPL-2.0-or-later
#include <cassert>
#include <sstream>
#include <limits>
#include "../native/ftebroom_preview_lighting.h"
int main(){
 PreviewLighting old;std::istringstream legacy("1 128");old.read(legacy);assert(old.enabled&&old.distance==128&&old.radius>1&&old.baked>0);
 PreviewLighting configured;configured.enabled=true;configured.distance=0;configured.radius=2;configured.energy=.4f;configured.ambient=.3f;configured.baked=.6f;
 std::ostringstream saved;configured.write(saved);PreviewLighting loaded;std::istringstream input(saved.str());loaded.read(input);
 assert(loaded.enabled&&loaded.distance==0&&loaded.radius==2&&loaded.energy==.4f&&loaded.ambient==.3f&&loaded.baked==.6f);
 loaded.distance=std::numeric_limits<float>::quiet_NaN();loaded.energy=-2;loaded.radius=100;loaded.baked=2;loaded.clamp();assert(loaded.distance==4096&&loaded.energy==0&&loaded.radius==4&&loaded.baked==1);
 assert(configured.command(false,"maps/preview.ent").find(" 0 \"maps/preview.ent\" ")!=std::string::npos);
 assert(configured.command(true,"maps/preview.ent").find(" 1 \"maps/preview.ent\" ")!=std::string::npos);
}
