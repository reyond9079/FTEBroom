#include "../native/ftebroom_map_profile.h"
#include <cassert>
int main(){using ftebroom::mapProfile;
 assert(mapProfile("// FTEBroom profile=quake1 format=valve220").profile==2);
 assert(mapProfile("// FTEBroom profile=when-the-sky-went-dark format=fte-terrain").terrain);
 assert(mapProfile("// FTEBroom profile=quake3 format=quake3").profile==0);
 assert(mapProfile("// Game: Quake\n// Format: Standard\n{\n}").profile==2);
 assert(mapProfile("// Game: Quake\r\n// Format: Valve\r\n{").format==1);
 assert(mapProfile("\xef\xbb\xbf// Game: Quake 3\n// Format: Quake3 (Valve)\n{").profile==0);
 assert(mapProfile("// Game: When the sky went dark\n// Format: Quake3\n{").profile==1);
 assert(mapProfile("// Game: Half-Life\n// Format: Valve\n{").profile==4);
 assert(mapProfile("// Game: Quake2RE\n// Format: Quake2 (Valve)\n{").profile==3);
 assert(mapProfile("// Game: objectivesurvive\n// Format: Quake2 (Valve)\n{").profile==3);
 assert(mapProfile("// Game: Quake2\n// Format: Valve\n{").profile==3);
 assert(mapProfile("// Format: Valve\n{").profile==-1);
 assert(mapProfile("{\n// Game: Quake\n// Format: Standard").profile==-1);
 assert(mapProfile("// ReyondBroom profile=when-the-sky-went-dark format=quake3\n// Game: Quake\n// Format: Standard").profile==1);
 assert(mapProfile("// ReyondBroom profile=when-the-sky-went-dark format=fte-terrain").terrain);
}
