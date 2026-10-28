#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <atomic>
#include <cassert>
#include "cover_pack.h"
#define ROOT "installed"
#define SCE_KERNEL_POWER_TICK_DEFAULT 0
static std::atomic<bool>stop_worker{false};
static void lock(){} static void unlock(){} static void sceKernelPowerTick(int){} static void update_message(const char*){}
static struct {unsigned covers_revision=0;}progress;
static bool no_space=false;
static void free_space(uint64_t){if(no_space)throw std::runtime_error("No space");}
static void pkgi_mkdirs(const char*p){std::filesystem::create_directories(p);}
static void pkgi_save(const std::string&p,const void*b,size_t n){FILE*f=fopen(p.c_str(),"wb");if(!f)throw std::runtime_error("write");bool ok=fwrite(b,1,n,f)==n;if(fclose(f))ok=false;if(!ok)throw std::runtime_error("write");}
typedef unsigned long uLong;
static uLong crc32(uLong previous,const unsigned char*b,unsigned n){uint32_t c=~(uint32_t)previous;for(unsigned i=0;i<n;i++){c^=b[i];for(int j=0;j<8;j++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
#define Z_NULL nullptr
struct SceIoStat{};
static int sceIoGetstat(const char*p,SceIoStat*){return std::filesystem::exists(p)?0:-1;}
static int sceIoRename(const char*a,const char*b){return rename(a,b);}
static int sceIoRemove(const char*p){return remove(p);}
static int sceIoSync(const char*,int){return 0;}
#include "../src/cover_commit.inc"
struct zip_t{std::string root;};typedef FILE zip_file_t;struct zip_stat_t{uint64_t size;};
static zip_t*open_vita_zip(const std::string&p){return new zip_t{p};}
static int zip_stat(zip_t*z,const char*p,int,zip_stat_t*s){auto path=z->root+"/"+p;if(!std::filesystem::exists(path))return -1;s->size=std::filesystem::file_size(path);return 0;}
static zip_file_t*zip_fopen(zip_t*z,const char*p,int){return fopen((z->root+"/"+p).c_str(),"rb");}
static long long zip_fread(zip_file_t*f,void*b,size_t n){return fread(b,1,n,f);}
static int zip_fclose(zip_file_t*f){return fclose(f);}
static void zip_discard(zip_t*z){delete z;}
static bool safe_relative(const std::string&p){return !p.empty() && p.find("..") ==std::string::npos && p.find(':')==std::string::npos;}
#include "../src/cover_install.inc"
static std::string read(const char*p){FILE*f=fopen(p,"rb");assert(f);std::string s;int c;while((c=fgetc(f))!=EOF)s.push_back((char)c);fclose(f);return s;}
int main(){
 std::filesystem::create_directories("archive/assets");const char*image="sample image";uint32_t crc=crc32(0,(const unsigned char*)image,strlen(image));
 char manifest[128];snprintf(manifest,sizeof(manifest),"PCSE00001.jpg\t%u\t%08x\n",(unsigned)strlen(image),crc);
 pkgi_save("archive/assets/manifest.tsv",manifest,strlen(manifest));pkgi_save("archive/assets/covers.bin",image,strlen(image));
 assert(!cover_pack_available());install_cover_archive("archive");assert(cover_pack_available());assert(read("installed/covers.bin")==image);assert(progress.covers_revision==1);
 pkgi_save("archive/assets/covers.bin","damaged blob",strlen(image));bool failed=false;try{install_cover_archive("archive");}catch(...){failed=true;}assert(failed && read("installed/covers.bin")==image);assert(!std::filesystem::exists("installed/covers.bin.new"));
 no_space=true;failed=false;try{install_cover_archive("archive");}catch(...){failed=true;}assert(failed && read("installed/covers.bin")==image);
 puts("Production cover install: streamed copy/index, CRC, publication/reload, corrupt source and no-space preservation passed (mock ZIP and Vita I/O).");
}