#include "plugins.h"
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cassert>
struct BundledPlugin{const char*file,*section,*sha256;};
static const BundledPlugin bundled_plugins[]={{"test.suprx","*ALL","test-hash"}};
static const char*psp_plugin_hash="psp-hash";
#define VS_BOOT_SHA256 "boot-hash"
static std::map<std::string,std::vector<unsigned char>>files;
static std::string canonical(std::string p){auto c=p.find(":/");if(c!=std::string::npos)p.erase(c+1,1);return p;}
static bool exists(const std::string&p){return files.count(canonical(p))!=0;}
static std::vector<unsigned char>read_file(const std::string&p){if(!exists(p))throw std::runtime_error("missing");return files.at(canonical(p));}
/* The readiness decision is tested separately from the production SHA-256 tests. */
static bool verify_hash(const std::vector<unsigned char>&b,const char*h){return std::string(b.begin(),b.end())==h;}
#include "../src/plugins_readiness.inc"
static void save(const std::string&p,const std::string&s){files[canonical(p)]={s.begin(),s.end()};}
static const char*config="*main\nur0:tai/VitaStation/VitaStationBoot.suprx\n*ALL\nur0:/tai/VitaStation/test.suprx\n";
static void setup(){files.clear();save("ur0:tai/config.txt",config);save("ur0:tai/VitaStation/VitaStationBoot.suprx","boot-hash");save("ur0:tai/VitaStation/test.suprx","test-hash");save("ux0:pspemu/seplugins/npdrm_free_mod.prx","psp-hash");for(const char*n:{"game.txt","vsh.txt"})save(std::string("ux0:pspemu/seplugins/")+n,"ms0:/seplugins/npdrm_free_mod.prx 1\n");}
int main(){
 setup();auto original=files;assert(configured_bundle_current());assert(files==original);
 save("ur0:tai/VitaStation/VitaStationBoot.suprx","stale bridge");assert(!configured_bundle_current());
 setup();files.erase("ur0:tai/VitaStation/test.suprx");assert(!configured_bundle_current());
 setup();save("ux0:pspemu/seplugins/game.txt","");assert(!configured_bundle_current());
 setup();save("ur0:tai/config.txt","*main\nur0:tai/VitaStation/VitaStationBoot.suprx\n*ALL\nur0:tai/updated-test.suprx\n");assert(!configured_bundle_current());
 setup();save("ur0:tai/config.txt","*main\nur0:tai/VitaStation/VitaStationBoot.suprx\n*ALL\n#VITASTATION_DISABLED ur0:tai/VitaStation/test.suprx\n");files.erase("ur0:tai/VitaStation/test.suprx");assert(configured_bundle_current());
 setup();save("ur0:tai/config.txt","*main\nur0:tai/VitaStation/VitaStationBoot.suprx\n*ALL\nur0:tai/test.suprx\n");save("ur0:tai/test.suprx","pre-existing updated version");assert(configured_bundle_current());
 puts("Read-only readiness: exact own bridge, canonical paths, missing/changed files, disabled choices, existing external updates and no filesystem writes passed.");
}
