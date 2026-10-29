#include "safety_backup.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <cassert>
#include <cstdio>
namespace fs=std::filesystem;
static void save(const char*p,const char*s){fs::create_directories(fs::path(p).parent_path());std::ofstream(p)<<s;}
static std::string read(const char*p){std::ifstream f(p);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(){
 save("testfs/ur0/tai/config.txt","*KERNEL\nur0:tai/original.skprx\numa0:plugins/external.skprx\n");save("testfs/ur0/tai/original.skprx","original plugin");save("testfs/uma0/plugins/external.skprx","external plugin");save("testfs/ux0/app/PSPEMUCFW/sce_module/adrenaline_user.suprx","original runtime");
 save("testfs/ur0/tai/missing.suprx","recover only when missing");
 char message[256];assert(safety_backup_ensure(message,sizeof(message)));std::string baseline=read("testfs/ux0/data/VitaStation/safety-backup/baseline/manifest.json");
 save("testfs/ur0/tai/config.txt","*KERNEL\nur0:tai/new.skprx\n");save("testfs/ur0/tai/new.skprx","new plugin");save("testfs/ur0/tai/original.skprx","modified plugin");save("testfs/uma0/plugins/external.skprx","modified external");save("testfs/ux0/app/PSPEMUCFW/sce_module/adrenaline_user.suprx","new runtime");save("testfs/ux0/pspemu/seplugins/game.txt","new config");
 fs::remove("testfs/ur0/tai/missing.suprx");save("testfs/ux0/app/PSPEMUCFW/sce_module/adrbubblebooter.suprx","new runtime component");assert(safety_backup_track_plugin("ux0:app/PSPEMUCFW/sce_module/adrbubblebooter.suprx",message,sizeof(message)));
 save("testfs/ur0/tai/owned.skprx","owned plugin");assert(safety_backup_track_plugin("ur0:tai/owned.skprx",message,sizeof(message)));
 assert(safety_backup_track_plugin("ur0:tai/original.skprx",message,sizeof(message)));
 save("testfs/ur0/tai/user-later.skprx","manual plugin later");
 save("testfs/ur0/tai/owned-newer.skprx","original owned");assert(safety_backup_track_plugin("ur0:tai/owned-newer.skprx",message,sizeof(message)));save("testfs/ur0/tai/owned-newer.skprx","updated by user");
 assert(safety_backup_ensure(message,sizeof(message)));assert(baseline==read("testfs/ux0/data/VitaStation/safety-backup/baseline/manifest.json"));assert(safety_backup_restore(message,sizeof(message)));
 assert(read("testfs/ur0/tai/config.txt")=="*KERNEL\nur0:tai/original.skprx\numa0:plugins/external.skprx\n");assert(read("testfs/ur0/tai/missing.suprx")=="recover only when missing");assert(read("testfs/ux0/app/PSPEMUCFW/sce_module/adrbubblebooter.suprx")=="new runtime component");
 assert(safety_backup_remove_additions(message,sizeof(message)));assert(!fs::exists("testfs/ur0/tai/owned.skprx"));assert(fs::exists("testfs/ur0/tai/user-later.skprx"));assert(read("testfs/ur0/tai/owned-newer.skprx")=="updated by user");
 assert(read("testfs/ur0/tai/original.skprx")=="modified plugin");assert(read("testfs/uma0/plugins/external.skprx")=="modified external");assert(read("testfs/ux0/app/PSPEMUCFW/sce_module/adrenaline_user.suprx")=="new runtime");assert(fs::exists("testfs/ur0/tai/new.skprx"));assert(!fs::exists("testfs/ux0/pspemu/seplugins/game.txt"));assert(fs::exists("testfs/ux0/data/VitaStation/safety-backup/before-uninstall/manifest.json"));
 save("testfs/ur0/tai/config.txt","current config must survive corrupt backup");for(auto&e:fs::directory_iterator("testfs/ux0/data/VitaStation/safety-backup/baseline")){if(e.path().filename()!="manifest.json"){std::ofstream(e.path())<<"damaged";break;}}
 assert(!safety_backup_restore(message,sizeof(message)));assert(read("testfs/ur0/tai/config.txt")=="current config must survive corrupt backup");
 puts("Snapshot: original configs restored; previous/updated/manual/Adrenaline modules kept; only recorded new plugin removed; missing original recovered; immutable backup and corruption guard passed.");
}