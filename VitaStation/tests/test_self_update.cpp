#include "self_update.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <cassert>
#include <cstdio>
static void save(const char*p,const std::string&s){std::filesystem::create_directories(std::filesystem::path(p).parent_path());std::ofstream(p,std::ios::binary)<<s;}
int main(){char error[256],version[32];assert(self_update_prepare("source","tickets/install.json",error,sizeof(error)));assert(self_update_verify("source","tickets/install.json",version,sizeof(version),error,sizeof(error)));assert(std::string(version)=="01.65");
 assert(self_update_copy("source","rollback",error,sizeof(error)));assert(!self_update_copy("source","rollback",error,sizeof(error)));assert(self_update_prepare("rollback","tickets/rollback.json",error,sizeof(error)));assert(self_update_verify("rollback","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));
 assert(self_update_verify("source","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));
 save("source/assets/logo.png","corrupt");assert(!self_update_verify("source","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));assert(!self_update_verify("source","tickets/install.json",version,sizeof(version),error,sizeof(error)));assert(self_update_verify("rollback","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));
 save("rollback/injected.bin","extra");assert(!self_update_verify("rollback","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));std::filesystem::remove("rollback/injected.bin");std::filesystem::remove("rollback/assets/logo.png");assert(!self_update_verify("rollback","tickets/rollback.json",version,sizeof(version),error,sizeof(error)));
 assert(!self_update_prepare("wrong","tickets/wrong.json",error,sizeof(error)));
 puts("Production self-update ticket: app ID/SFO/SELF, complete resources, SHA-256, backup copy, corruption/extra/missing resource and wrong app rejection passed.");
}
