#include "catalog.h"
#include "store.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <cassert>
#include <iostream>
namespace fs=std::filesystem;
using SceUID=int;using SceSize=unsigned;
enum {SCE_O_RDONLY=1,SCE_O_WRONLY=2,SCE_O_CREAT=4,SCE_O_EXCL=8};
static bool cancel_job=false,stop_worker=false;static int nextfd=1,rename_calls=0,fail_rename=-1;
static std::map<int,std::fstream*> handles;
static fs::path host(const std::string&p){return fs::path("fs")/p.substr(0,3)/p.substr(4);}
static int sceIoOpen(const char*p,int flags,int){auto path=host(p);if((flags&SCE_O_EXCL) && fs::exists(path))return -1;auto*f=new std::fstream(path, std::ios::binary | (flags&SCE_O_RDONLY?std::ios::in:(std::ios::out|std::ios::trunc)));if(!*f){delete f;return -1;}handles[++nextfd]=f;return nextfd;}
static int sceIoRead(int fd,void*data,unsigned n){auto*f=handles.at(fd);f->read((char*)data,n);return f->bad()?-1:(int)f->gcount();}
static int sceIoWrite(int fd,const void*data,unsigned n){auto*f=handles.at(fd);f->write((const char*)data,n);return *f?(int)n:-1;}
static int sceIoClose(int fd){delete handles.at(fd);handles.erase(fd);return 0;}
static int sceIoSyncByFd(int,int){return 0;}
static int sceIoRemove(const char*p){return fs::remove(host(p))?0:-1;}
static uint64_t sceKernelGetProcessTimeWide(){static uint64_t n=0;return ++n;}
static void check(int code,const char*text){if(code<0)throw std::runtime_error(text);}
static bool safe_relative(const std::string&p){if(p.empty() || p.front()=='/' || p.find(':')!=std::string::npos || p.find('\\')!=std::string::npos)return false;for(const auto&part:fs::path(p))if(part==".." || part==".")return false;return true;}
enum class InodeType{File,Directory,NotExist};
static InodeType pkgi_get_inode_type(const std::string&p){return !fs::exists(host(p))?InodeType::NotExist:fs::is_directory(host(p))?InodeType::Directory:InodeType::File;}
static bool pkgi_file_exists(const std::string&p){return fs::is_regular_file(host(p));}
static void pkgi_mkdirs(const char*p){fs::create_directories(host(p));}
static std::vector<std::string> pkgi_list_dir_contents(const std::string&p){std::vector<std::string> result;for(auto&e:fs::directory_iterator(host(p)))result.push_back(e.path().filename().string());std::sort(result.begin(),result.end());return result;}
static void pkgi_rename(const std::string&a,const std::string&b){if(++rename_calls==fail_rename)throw std::runtime_error("Injected rename failure");fs::rename(host(a),host(b));}
static void pkgi_delete_dir(const std::string&p){fs::remove_all(host(p));}
static void download_file(const CatalogPackage&,const std::string&){throw std::runtime_error("Not used");}
static void extract_archive(const std::string&,const std::string&){throw std::runtime_error("Not used");}
static void phase(StorePhase,const char*){}
#include "../third_party/psvitaalive/mediafire.hpp"
#include "../src/store_merge.inc"
#include "../src/store_alive.inc"
extern "C" bool plugins_install_catalog(const char*,const char*,const char*,char*,size_t){return false;}
static void put(const std::string&p,const char*text){fs::create_directories(host(p).parent_path());std::ofstream(host(p))<<text;}
static std::string get(const std::string&p){std::ifstream f(host(p));return std::string(std::istreambuf_iterator<char>(f),{});}
template<class F>static void fails(F function){bool threw=false;try{function();}catch(const std::exception&){threw=true;}assert(threw);}
int main(){
 CatalogPackage p{};strcpy(p.title_id,"TEST00001");strcpy(p.extract_path,"ux0:data/");assert(data_destination(p)=="ux0:data");strcpy(p.extract_path,"ux0:app/");assert(data_destination(p)=="ux0:app/TEST00001");
 for(const char*path:{"ur0:tai/","ux0:data/../tai/","ux0:app/OTHER0001/","ux0:data/vitastation/"}){strcpy(p.extract_path,path);fails([&]{data_destination(p);});}
 put("ux0:stage/a","new a");put("ux0:stage/sub/b","new b");put("ur0:data/a","old a");put("ur0:data/sub/b","old b");
 merge_homebrew_data("ux0:stage","ur0:data");assert(get("ur0:data/a")=="new a" && get("ur0:data/sub/b")=="new b");
 put("ur0:data/a","old a");put("ur0:data/sub/b","old b");rename_calls=0;fail_rename=4;
 fails([]{merge_homebrew_data("ux0:stage","ur0:data");});assert(get("ur0:data/a")=="old a" && get("ur0:data/sub/b")=="old b" && handles.empty());
 fail_rename=-1;cancel_job=true;fails([]{merge_homebrew_data("ux0:stage","ur0:data");});cancel_job=false;
 put("ux0:protected/VitaStation/settings","attack");fails([]{merge_homebrew_data("ux0:protected","ux0:data");});assert(!fs::exists(host("ux0:data/VitaStation/settings")));
 assert(alive_mediafire::findDirectFromHtml("<a id='downloadButton' href='https://download1.mediafire.com/x/test.vpk'>Download</a>")=="https://download1.mediafire.com/x/test.vpk");
 assert(alive_mediafire::findDirectFromHtml("<script>alert('no url')</script>").empty());
 std::cout<<"Alive file engine: partition copy, commit rollback, cancellation, protected targets and MediaFire parsing passed.\n";
}

