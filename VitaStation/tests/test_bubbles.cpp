#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>
#include <mutex>
#include <cassert>
#include "psp_media.h"
#include "settings.h"
namespace fs=std::filesystem;
static long long epoch=1000000;static int rename_count=0,rename_fail=-1,promotions=0;
static fs::path host(const std::string&p){auto colon=p.find(':');return colon==std::string::npos?fs::path(p):fs::path("fs")/p.substr(0,colon)/p.substr(colon+1);}
static FILE*mock_open(const char*p,const char*mode){return std::fopen(host(p).string().c_str(),mode);}
struct SceIoStat{};struct SceRtcTick{uint64_t tick;};
static int sceIoGetstat(const char*p,SceIoStat*){return fs::exists(host(p))?0:-1;}
static int sceIoRename(const char*a,const char*b){if(++rename_count==rename_fail)return -1;std::error_code e;fs::rename(host(a),host(b),e);return e?-1:0;}
static int sceIoRemove(const char*p){return fs::remove(host(p))?0:-1;}
static int sceIoSync(const char*,int){return 0;}
static int sceRtcGetCurrentTick(SceRtcTick*t){t->tick=(uint64_t)epoch*1000000+100;return 0;}
static uint64_t sceKernelGetSystemTimeWide(){return 100;}
static unsigned long crc32(unsigned long prior,const unsigned char*p,unsigned n){uint32_t c=(uint32_t)prior^0xffffffff;for(unsigned i=0;i<n;i++){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320:0);}return c^0xffffffff;}
extern "C" void settings_ensure_dir(const char*p){fs::create_directories(host(p));}
extern "C" void installation_mark_dirty(){}
void vs_prepare_promoter(){}
void vs_prepare_bubble_head(const std::string&root,const char*){fs::create_directories(host(root+"/sce_sys/package"));std::ofstream(host(root+"/sce_sys/package/head.bin"))<<"header";}
void vs_finish_bubble_staging(const std::string&root){fs::remove_all(host(root));}
static int scePromoterUtilityPromotePkg(const char*p,int){assert(fs::exists(host(std::string(p)+"/sce_sys/package/head.bin")));fs::copy(host(p),host("ux0:app/R12345678"),fs::copy_options::recursive);promotions++;return 0;}
#define fopen mock_open
#define remove mock_remove
static int mock_remove(const char*p){return sceIoRemove(p);}
#include "../src/psp_bubbles.cpp"
#undef fopen
#undef remove
static std::string get(const std::string&p){std::ifstream f(host(p),std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});}
static void put(const std::string&p,const std::string&s){fs::create_directories(host(p).parent_path());std::ofstream(host(p),std::ios::binary)<<s;}
static void originals(){fs::remove_all("fs/ux0");fs::remove_all("vs-data");fs::create_directories("vs-data");for(const auto*n:{"adrenaline_kernel.skprx","adrenaline_user.suprx","adrenaline_vsh.suprx"}){std::string p="ux0:app/PSPEMUCFW/sce_module/";fs::create_directories(host(p));fs::copy_file(fs::path("original")/n,host(p+n));}rename_count=0;rename_fail=-1;}
int main(){
 char message[256];originals();rename_fail=2;assert(psp_prepare_runtime(message,sizeof(message))==-1);for(const auto*n:{"adrenaline_kernel.skprx","adrenaline_user.suprx","adrenaline_vsh.suprx"})assert(get(std::string("ux0:app/PSPEMUCFW/sce_module/")+n)==get(std::string("original/")+n));
 originals();assert(psp_prepare_runtime(message,sizeof(message))==1);for(unsigned i=0;i<5;i++)assert(crc(read(std::string(module_root)+module_names[i]))==runtime_crc[i]);assert(psp_prepare_runtime(message,sizeof(message))==1);
 AppItem item{};strcpy(item.title_id,"R12345678");strcpy(item.title,"PSP test title");strcpy(item.media_path,"ux0:pspemu/ISO/test.iso");item.media_raw=true;put(item.media_path,"iso fixture");assert(!psp_create_bubble(&item,message,sizeof(message)) && !promotions);
 epoch+=100;assert(psp_prepare_runtime(message,sizeof(message))==0);assert(psp_create_bubble(&item,message,sizeof(message)) && promotions==1);auto boot=get("ux0:app/R12345678/data/boot.bin");assert(boot.size()==320 && !strcmp(boot.data()+64,item.media_path));auto sfo=get("ux0:app/R12345678/sce_sys/param.sfo");assert(sfo.find(item.title_id)!=std::string::npos && sfo.find(item.title)!=std::string::npos);assert(psp_create_bubble(&item,message,sizeof(message)) && promotions==1);
 put("ux0:app/R12345678/data/boot.bin",std::string(320,'x'));assert(!psp_create_bubble(&item,message,sizeof(message)));
 originals();put("ux0:app/PSPEMUCFW/sce_module/adrenaline_user.suprx","unknown custom module");assert(psp_prepare_runtime(message,sizeof(message))==-1 && get("ux0:app/PSPEMUCFW/sce_module/adrenaline_user.suprx")=="unknown custom module");
 puts("Actual PSP setup: guarded version check, module rollback, backups, reboot gate, bubble SFO/path generation, promotion and collision rejection passed (mocked console calls).");
}
