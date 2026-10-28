"""Exercise the production property aggregation cache and its invalidation keys."""
from pathlib import Path
import argparse, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--compiler',default='c++');a=p.parse_args()
source=(Path(__file__).resolve().parents[1]/'native/ftebroom_property_table.h').read_text(encoding='utf8')
row=source[source.index('struct PropertyRow'):source.index('static void entityPropertyTable()')]
block=source[source.index('  static std::map<std::string, ftebroom::EntityProperty> definitions;'):source.index('  static std::string selected, editing, pending;')]
prefix=r"""
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
namespace ftebroom { struct EntityProperty {std::string value;}; struct EntityDefinition {std::map<std::string,EntityProperty> properties;}; }
static std::map<int,std::map<std::string,std::string>> entityProperties;
static std::map<std::string,ftebroom::EntityDefinition> entityDefinitions;
static int entitySelectionEpoch=1,entityDefinitionEpoch=1,organizationMapEpoch=1;
"""
body=prefix+row+r"""
struct Snapshot {std::vector<PropertyRow> rows;std::vector<int> targets;std::map<std::string,unsigned> protectedCounts;};
static Snapshot aggregate(bool showDefaults) {
"""+block+r"""
return {rows,now,protectedCounts};
}
static const PropertyRow& get(const Snapshot& s,const char *key){for(auto& r:s.rows)if(r.key==key)return r;assert(false);return s.rows[0];}
int main(){
 entityDefinitions["misc_drip"].properties["speed"].value="100";
 entityProperties={{1,{{"classname","misc_drip"},{"origin","0 0 0"},{"message","same"},{"_tb_protected_properties","message;speed"}}},{2,{{"classname","misc_drip"},{"origin","16 0 0"},{"message","same"}}}};
 auto initial=aggregate(true);assert(initial.targets==std::vector<int>({1,2}));assert(get(initial,"origin").mixed);assert(!get(initial,"message").mixed);assert(get(initial,"speed").value=="100"&&!get(initial,"speed").explicitValue&&!get(initial,"speed").complete);assert(initial.protectedCounts["message"]==1);
 entityProperties[1]["message"]="changed";assert(get(aggregate(true),"message").value=="same");
 ++entitySelectionEpoch;auto edited=aggregate(true);assert(get(edited,"message").value=="changed"&&get(edited,"message").mixed);
 auto hidden=aggregate(false);for(auto& r:hidden.rows)assert(r.key!="speed");
 entityDefinitions["misc_drip"].properties["speed"].value="200";++entityDefinitionEpoch;assert(get(aggregate(true),"speed").value=="200");
 entityProperties[1]["message"]=entityProperties[2]["message"]="new map";++organizationMapEpoch;assert(!get(aggregate(true),"message").mixed&&get(aggregate(true),"message").value=="new map");
 entityProperties[1]["model"]="*1";++entitySelectionEpoch;assert(get(aggregate(true),"model").internal);
 entityDefinitions["worldspawn"].properties["message"].value="world default";entityProperties={{0,{{"classname","worldspawn"}}}};++entitySelectionEpoch;++entityDefinitionEpoch;auto world=aggregate(true);assert(world.targets==std::vector<int>({0})&&get(world,"message").value=="world default");
 puts("PASS property cache: publication, definitions, defaults, map changes, mixed values, protected keys and worldspawn");
}
"""
with tempfile.TemporaryDirectory(prefix='reyond-property-cache-') as temp:
 src=Path(temp)/'test.cpp';exe=Path(temp)/'test.exe';src.write_text(body,encoding='utf8')
 subprocess.run([a.compiler,'-std=c++17','-O2',str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
