#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cctype>
#include <cassert>
#include <iostream>
namespace fs=std::filesystem;
using SceUID=int;using SceOff=int64_t;using uLong=unsigned long;
#define SCE_SEEK_SET 0
#define Z_NULL nullptr
#define ROOT "ux0:data/VitaStation/covers"
#define SCE_O_RDONLY 1
#define SCE_O_WRONLY 2
#define SCE_O_CREAT 4
#define SCE_O_TRUNC 8
struct SceIoStat{int64_t st_size;};
static unsigned char buffer[65536];static int nextfd=0,rename_count=0,rename_fail=-1;
static std::map<int,std::fstream*> handles;static int read_open_successes=0;
static fs::path host(const std::string&p){return fs::path("fs")/p.substr(0,p.find(':'))/p.substr(p.find(':')+1);}
static int sceIoOpen(const char*p,int mode,int){auto*f=new std::fstream(host(p),std::ios::binary | ((mode&SCE_O_RDONLY)?std::ios::in:(std::ios::out|std::ios::trunc)));if(!*f){delete f;return -1;}if(mode&SCE_O_RDONLY)read_open_successes++;handles[++nextfd]=f;return nextfd;}
static int sceIoRead(int fd,void*p,unsigned n){auto*f=handles.at(fd);f->read((char*)p,n);return f->bad()?-1:(int)f->gcount();}
static int sceIoWrite(int fd,const void*p,unsigned n){auto*f=handles.at(fd);f->write((const char*)p,n);return *f?(int)n:-1;}
static int sceIoClose(int fd){delete handles.at(fd);handles.erase(fd);return 0;}
static SceOff sceIoLseek(int fd,SceOff offset,int){auto*f=handles.at(fd);f->clear();f->seekg(offset);return *f?offset:-1;}
static int sceIoRemove(const char*p){return fs::remove(host(p))?0:-1;}
static int sceIoGetstat(const char*p,SceIoStat*s){if(!fs::exists(host(p)))return -1;s->st_size=fs::file_size(host(p));return 0;}
static int sceIoRename(const char*a,const char*b){if(++rename_count==rename_fail)return -1;std::error_code e;fs::rename(host(a),host(b),e);return e?-1:0;}
static uint64_t sceKernelGetProcessTimeWide(){static uint64_t n=0;return ++n;}
static uLong crc32(uLong prior,const unsigned char*p,unsigned n){uint32_t c=(uint32_t)prior^0xffffffff;for(unsigned i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^((c&1)?0xedb88320:0);}return c^0xffffffff;}
static FILE* platform_fopen(const char*p,const char*mode){auto path=host(p);return std::fopen(path.string().c_str(),mode);}
#define fopen platform_fopen
#include "../src/install_core.h"
#undef fopen
static int sceIoSync(const char*,int){return 0;}
#include "../src/block_commit.h"
static void put(const std::string&p,const char*s){fs::create_directories(host(p).parent_path());std::ofstream(host(p),std::ios::binary)<<s;}
static std::string get(const std::string&p){std::ifstream f(host(p),std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(){
 assert(crc32(0,(const unsigned char*)"123456789",9)==0xcbf43926);
 assert(safe_name("hb_0123456789abcdef_001.png") && !safe_name("../evil.png") && !safe_name("x/y.png") && !safe_name("file.txt"));
 char line[]="BREW00001.png\t3\t6be34445\n",name[96];uint64_t size;uint32_t crc;assert(parse(line,name,&size,&crc) && size==3);
 fs::create_directories(host(ROOT));put("app0:assets/covers/TEST00001.png","new");put(ROOT "/TEST00001.png","old");
 uint32_t sum=(uint32_t)crc32(0,(const unsigned char*)"new",3);
 assert(copy("TEST00001.png",3,sum)==0 && get(ROOT "/TEST00001.png")=="new");
 assert(get(ROOT "/TEST00001.png.before-coverpack-1.bak")=="old");
 assert(copy("TEST00001.png",3,sum)==1 && handles.empty());
 put(ROOT "/TEST00001.png","old2");rename_count=0;rename_fail=2;
 assert(copy("TEST00001.png",3,sum)==-5 && get(ROOT "/TEST00001.png")=="old2");
 rename_fail=-1;assert(copy("TEST00001.png",3,0)==-3 && get(ROOT "/TEST00001.png")=="old2");
 put("app0:assets/covers.bin","newXYZ");source_blob=sceIoOpen("app0:assets/covers.bin",SCE_O_RDONLY,0);source_position=0;source_offset=3;
 int opens_before=read_open_successes;uint32_t xyz=(uint32_t)crc32(0,(const unsigned char*)"XYZ",3);assert(copy("TEST00002.png",3,xyz)==0 && get(ROOT "/TEST00002.png")=="XYZ");assert(read_open_successes==opens_before);
 source_offset=0;assert(copy("TEST00003.png",3,sum)==0 && get(ROOT "/TEST00003.png")=="new");
 source_offset=3;assert(copy("TEST00002.png",3,xyz)==1);assert(copy("TEST00004.png",4,xyz)==-3);
 sceIoClose(source_blob);source_blob=-1;assert(handles.empty());
 assert(marker());auto original=get(ROOT "/offline.enabled");assert(marker() && get(ROOT "/offline.enabled")==original);
 assert(get(ROOT "/TEST00001.png")=="old2" && fs::exists(host(ROOT "/TEST00001.png.before-coverpack-1.bak")));
 put(ROOT "/covers.bin","oldblob");put(ROOT "/manifest.tsv","oldindex");
 put(ROOT "/covers.bin.new","newblob");put(ROOT "/manifest.tsv.new","newindex");
 rename_count=0;rename_fail=4;assert(!commit_pack());
 assert(get(ROOT "/covers.bin")=="oldblob" && get(ROOT "/manifest.tsv")=="oldindex");
 put(ROOT "/covers.bin.new","newblob");put(ROOT "/manifest.tsv.new","newindex");
 rename_count=0;rename_fail=-1;assert(commit_pack());
 assert(get(ROOT "/covers.bin")=="newblob" && get(ROOT "/manifest.tsv")=="newindex");
 assert(get(ROOT "/covers.bin.previous")=="oldblob" && get(ROOT "/manifest.tsv.previous")=="oldindex");
 std::cout<<"Bulk pack paired commit and rollback passed.\n";
 std::cout<<"Cover installer: source CRC, resumable destination CRC, one shared payload handle, skipped slices, short reads, preserved originals, rollback and permanent marker passed.\n";
}
