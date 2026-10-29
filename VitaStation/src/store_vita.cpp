#include "game_support.h"
#include "safety_backup.h"
#include "plugins.h"
#include "../third_party/cJSON/cJSON.h"
#include "cover_pack.h"
#include <zlib.h>
#include "updater.h"
#include "version.h"
/* Native VitaStation adapter. PKG decoding: third_party/usagi (BSD-2-Clause).
 * Homebrew head.bin generation adapted from VitaShell, TheFloW (GPL-3.0-or-later).
 * See third_party/vitashell/LICENSE and THIRD_PARTY.md. */
#include "store.h"
#include "installation_watch.h"
#include "dlc_plan.h"
#include "system_info.h"
#include "../third_party/usagi/download.hpp"
#include "../third_party/usagi/bgdl.hpp"
#include <psp2/vshbridge.h>
#include <psp2common/npdrm.h>
#include "../third_party/usagi/vitahttp.hpp"
#include "../third_party/usagi/file.hpp"
#include "../third_party/usagi/utils.hpp"
#include "../third_party/usagi/zrif.hpp"
#include "../third_party/vitashell/base_head_bin.h"
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/devctl.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>
#include <psp2/libssl.h>
#include <psp2/promoterutil.h>
#include <openssl/sha.h>
#include <zip.h>
#include <atomic>
#include <curl/curl.h>
#include <mutex>
#include <cctype>
#include <cstring>
#include <algorithm>
#include "../third_party/psvitaalive/mediafire.hpp"

static SceUID lock_id=-1,wake_id=-1,thread_id=-1;
static SceUID cover_wake_id=-1,cover_thread_id=-1;
static std::mutex network_init_mutex;
static std::atomic<bool> stop_worker{false},cancel_job{false};
static StoreProgress progress{};
static UpdateStatus update_status{};
static GameSupportStatus game_status{};
static AppItem game_request{};static int game_action=0,game_selection=0;static bool game_pending=false;
static char background_request[32]{},background_history[64][32]{};static unsigned background_next=0;
static std::string mod_repos[5];
static bool update_pending=false,update_download=false,update_busy=false;
static CatalogPackage queue[8];
static char active_content_id[64]{},active_title_id[10]{},active_url[1024]{};
static unsigned first=0,count=0;
struct CoverRequest{CatalogType type;char id[64];char url[1024];};
static CoverRequest cover_queue[16];
static unsigned cover_first=0,cover_count=0,cover_history_next=0;
static char cover_history[512][64]{};
static uint8_t net_pool[1024*1024];
static bool network_ready=false,installer_ready=false,legacy_http_ready=false,offline_covers=false;
static bool homebrew_refresh_pending=false,homebrew_refresh_started=false;
extern "C" {
#include "log.h"
}
static void lock(){sceKernelLockMutex(lock_id,1,nullptr);}
static void unlock(){sceKernelUnlockMutex(lock_id,1);}
static void phase(StorePhase s,const char*message){lock();progress.phase=s;snprintf(progress.message,sizeof(progress.message),"%s",message);unlock();}
uint32_t pkgi_time_msec(){return (uint32_t)(sceKernelGetProcessTimeWide()/1000);}
void vs_engine_log(const char*message){vs_log("store",message,0);}
static void check(int code,const char*operation){if(code<0)throw std::runtime_error(fmt::format("{}: 0x{:08X}",operation,(uint32_t)code));}
static bool safe_relative(const std::string&p){
    if(p.empty() || p.size()>240 || p.front()=='/' || p.find(':')!=std::string::npos || p.find('\\')!=std::string::npos || p.find('\0')!=std::string::npos)return false;
    for(size_t pos=0;pos<p.size();){size_t end=p.find('/',pos);auto part=p.substr(pos,end-pos);if(part==".." || part==".")return false;if(end==std::string::npos)break;
        pos=end+1;}
    return true;
}
/* Recursive deletion is restricted to this application's private staging area. */
void pkgi_delete_dir(const std::string&path){
    const std::string prefix="ux0:data/VitaStation/pkg-staging/";
    if(path.compare(0,prefix.size(),prefix) || !safe_relative(path.substr(prefix.size())))throw std::runtime_error("Cleanup outside staging refused");
    auto type=pkgi_get_inode_type(path);if(type==InodeType::NotExist)return;
    if(type==InodeType::File){check(sceIoRemove(path.c_str()),"Remove staging file");return;}
    for(const auto&name:pkgi_list_dir_contents(path)){if(!safe_relative(name) || name.find('/')!=std::string::npos)throw std::runtime_error("Invalid staging entry");pkgi_delete_dir(path+"/"+name);}
    check(sceIoRmdir(path.c_str()),"Remove staging directory");
}
static void init_network(){
    std::lock_guard<std::mutex> guard(network_init_mutex);
    if(network_ready)return;
    check(sceSysmoduleLoadModule(SCE_SYSMODULE_NET),"Load network");
    SceNetInitParam n{};n.memory=net_pool;n.size=sizeof(net_pool);check(sceNetInit(&n),"Initialize network");
    check(sceNetCtlInit(),"Initialize network control");
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)throw std::runtime_error("Initialize HTTPS client failed");
    network_ready=true;
}
static void init_services(){
    init_network();if(installer_ready)return;
    uint32_t args[]={0x400000,0xea60,0x40000,0,1,0};int result=0;SceSysmoduleOpt opt{};opt.result=&result;
    check(sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,sizeof(args),args,&opt),"Load PAF");
    check(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL),"Load installer");
    check(scePromoterUtilityInit(),"Initialize installer");installer_ready=true;
}
void vs_prepare_promoter(){init_services();}
static void init_legacy_http(){
    if(legacy_http_ready)return;
    init_network();
    check(sceSysmoduleLoadModule(SCE_SYSMODULE_SSL),"Load installer SSL");
    check(sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP),"Load installer HTTP");
    check(sceSslInit(1024*1024),"Initialize installer SSL");
    check(sceHttpInit(1024*1024),"Initialize installer HTTP");
    legacy_http_ready=true;
}
extern "C" size_t system_network_describe(char lines[6][128]){
    try{
        init_network();int state=-1;check(sceNetCtlInetGetState(&state),"Read network state");
        const char*states[]={"Desligada","A ligar","A obter endereco IP","Ligada"};size_t count=0;
        snprintf(lines[count++],128,"Ligacao: %s",state>=0 && state<=3?states[state]:"Estado desconhecido");
        SceNetCtlInfo info{};
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_SSID,&info)>=0)snprintf(lines[count++],128,"Rede Wi-Fi: %.32s",info.ssid);
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS,&info)>=0)snprintf(lines[count++],128,"Endereco IP: %.15s",info.ip_address);
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_PRIMARY_DNS,&info)>=0)snprintf(lines[count++],128,"DNS principal: %.15s",info.primary_dns);
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_SECONDARY_DNS,&info)>=0)snprintf(lines[count++],128,"DNS secundario: %.15s",info.secondary_dns);
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_HTTP_PROXY_CONFIG,&info)>=0)snprintf(lines[count++],128,"Proxy HTTP: %s",info.http_proxy_config?"Ativado":"Desativado");
        return count;
    }catch(const std::exception&e){vs_log("network-info",e.what(),-1);return 0;}
}
static std::string sfo_string(const std::vector<uint8_t>&data,const char*key){
    if(data.size()<20 || get32le(data.data())!=0x46535000)throw std::runtime_error("Invalid SFO");
    uint32_t keys=get32le(data.data()+8),values=get32le(data.data()+12),n=get32le(data.data()+16);
    if(n>4096 || uint64_t(n)*16+20>data.size())throw std::runtime_error("Invalid SFO index");
    for(uint32_t i=0;i<n;i++){
        const uint8_t*e=data.data()+20+i*16;uint64_t ko=uint64_t(keys)+get16le(e),vo=uint64_t(values)+get32le(e+12);uint32_t len=get32le(e+4);
        if(ko>=data.size() || vo>data.size() || len>data.size()-vo)throw std::runtime_error("Invalid SFO bounds");
        const char*k=(const char*)data.data()+ko;const char*ke=(const char*)memchr(k,0,data.size()-ko);if(!ke)throw std::runtime_error("Invalid SFO key");
        if(!strcmp(k,key)){
            if(get16le(e+2)!=0x204 || !len)throw std::runtime_error("Invalid SFO string");
            const char*v=(const char*)data.data()+vo;const char*end=(const char*)memchr(v,0,len);if(!end)throw std::runtime_error("Invalid SFO value");return std::string(v,end);
        }
    }return {};
}
static void fpkg_hash(const uint8_t*data,size_t size,uint8_t*out){
    uint8_t digest[20],buffer[64]{};SHA1(data,size,digest);memcpy(buffer,digest+4,8);memcpy(buffer+8,digest+4,8);memcpy(buffer+16,digest+12,4);
    buffer[20]=digest[16];buffer[21]=digest[1];buffer[22]=digest[2];buffer[23]=digest[3];memcpy(buffer+24,buffer+16,8);SHA1(buffer,64,digest);memcpy(out,digest,16);
}
static void homebrew_head(const std::string&root,const CatalogPackage&p){
    auto sfo=pkgi_load(root+"/sce_sys/param.sfo");auto id=sfo_string(sfo,"TITLE_ID");
    if(id.size()!=9 || id=="VTST00001" || id.compare(0,4,"NPXS")==0 || !std::all_of(id.begin(),id.end(),[](char c){return std::isalnum((unsigned char)c);}))throw std::runtime_error("Invalid or protected homebrew Title ID");
    if(p.title_id[0] && id!=p.title_id)throw std::runtime_error("The package Title ID does not match the catalog");
    auto content=sfo_string(sfo,"CONTENT_ID");if(content.empty())content=fmt::format("EP9000-{}_00-XXXXXXXXXXXXXXXX",id);
    if(content.size()>47)throw std::runtime_error("Invalid content identifier");
    std::vector<uint8_t> head(base_head_bin,base_head_bin+sizeof(base_head_bin));memset(head.data()+0x30,0,48);memcpy(head.data()+0x30,content.c_str(),content.size());
    auto hash=[&](size_t off,size_t len,size_t dest){if(off>head.size() || len>head.size()-off || dest>head.size()-16)throw std::runtime_error("Invalid homebrew header template");fpkg_hash(head.data()+off,len,head.data()+dest);};
    size_t len=get32be(head.data()+0xd0);hash(0,len,len);size_t off=get32be(head.data()+8);len=get32be(head.data()+0x10);if(len<64)throw std::runtime_error("Invalid header");hash(off,len-64,get32be(head.data()+0xd4));len=get32be(head.data()+0xe8);hash(0,len,len);
    pkgi_mkdirs((root+"/sce_sys/package").c_str());pkgi_save(root+"/sce_sys/package/head.bin",head.data(),head.size());
}
void vs_prepare_bubble_head(const std::string&root,const char*id){CatalogPackage p{};snprintf(p.title_id,sizeof(p.title_id),"%s",id);homebrew_head(root,p);}
void vs_finish_bubble_staging(const std::string&root){if(pkgi_get_inode_type(root)!=InodeType::NotExist)pkgi_delete_dir(root);}
static void free_space(uint64_t needed){
    SceIoDevInfo info{};
    check(sceIoDevctl("ux0:",0x3001,nullptr,0,&info,sizeof(info)),"Read free space");
    if(info.free_size<0 || (uint64_t)info.free_size<needed+16*1024*1024)throw std::runtime_error("Not enough free space on ux0:");
}
static void progress_bytes(uint64_t done,uint64_t total){lock();progress.downloaded=done;progress.total=total;unlock();}
#include "store_tls.inc"
static CURL* https_client(const std::string&url);
static std::vector<uint8_t> https_bytes(const std::string&url,size_t limit);
#include "store_zip.inc"
struct FileTransfer{void*file;sha256_ctx sha;uint64_t done=0,limit=(uint64_t(32)<<30);bool failed=false;};
static size_t file_write(char*data,size_t size,size_t count,void*context){
    auto*t=(FileTransfer*)context;if(size && count>SIZE_MAX/size)return 0;size_t n=size*count;
    if(cancel_job || stop_worker || n>t->limit-t->done)return 0;
    try{pkgi_write(t->file,data,n);sha256_update(&t->sha,(const uint8_t*)data,n);t->done+=n;}catch(...){t->failed=true;return 0;}return n;
}
static int transfer_progress(void*,curl_off_t total,curl_off_t done,curl_off_t,curl_off_t){progress_bytes(done,total>0?total:0);return cancel_job || stop_worker;}
static std::string resolved_url(const char*url){
    std::string result=url;
    if(alive_mediafire::toLowerAscii(result).find("mediafire.com/file/")!=std::string::npos){
        auto bytes=https_bytes(result,1024*1024);result=alive_mediafire::findDirectFromHtml(std::string(bytes.begin(),bytes.end()));
        if(result.empty())throw std::runtime_error("Nao foi possivel resolver a descarga MediaFire.");
    }return result;
}
static void download_file(const CatalogPackage&p,const std::string&archive){
    auto url=resolved_url(p.url);CURL*h=https_client(url);char detail[CURL_ERROR_SIZE]{};curl_easy_setopt(h,CURLOPT_ERRORBUFFER,detail);FileTransfer transfer{};sha256_init(&transfer.sha);
    try{transfer.file=pkgi_create(archive);}catch(...){curl_easy_cleanup(h);throw;}
    curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,file_write);curl_easy_setopt(h,CURLOPT_WRITEDATA,&transfer);
    curl_easy_setopt(h,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(h,CURLOPT_XFERINFOFUNCTION,transfer_progress);
    auto code=curl_easy_perform(h);curl_easy_cleanup(h);pkgi_close(transfer.file);
    if(code!=CURLE_OK || transfer.failed || !transfer.done){pkgi_rm(archive.c_str());throw std::runtime_error(transfer_error(code,detail));}
    if(p.digest[0]){uint8_t sum[32];sha256_finish(&transfer.sha,sum);for(unsigned i=0;i<32;i++)if(sum[i]!=(hexvalue(p.digest[i*2])*16+hexvalue(p.digest[i*2+1]))){pkgi_rm(archive.c_str());throw std::runtime_error("VPK SHA-256 mismatch");}}
}
static void extract_archive(const std::string&archive,const std::string&root){
    uint8_t buffer[64*1024];
    phase(STORE_DOWNLOADING,"A preparar os ficheiros do homebrew...");
    zip_t*z=open_vita_zip(archive);
    try{
        zip_int64_t entries=zip_get_num_entries(z,0);if(entries<=0 || entries>65536)throw std::runtime_error("Invalid VPK entry count");uint64_t expanded=0;
        for(zip_uint64_t i=0;i<(zip_uint64_t)entries;i++){zip_stat_t st;zip_stat_init(&st);if(zip_stat_index(z,i,0,&st)<0 || !st.name || !safe_relative(st.name))throw std::runtime_error("Unsafe VPK filename");
            if(st.size>(uint64_t(8)<<30)-expanded)throw std::runtime_error("VPK expanded size too large");
            expanded+=st.size;if(expanded>(uint64_t(8)<<30))throw std::runtime_error("VPK expanded size too large");}
        free_space(expanded);
        for(zip_uint64_t i=0;i<(zip_uint64_t)entries;i++){
            if(cancel_job)throw std::runtime_error("Download canceled");
            zip_stat_t st;zip_stat_index(z,i,0,&st);std::string path=root+"/"+st.name;
            zip_uint8_t opsys;zip_uint32_t attributes=0;if(zip_file_get_external_attributes(z,i,0,&opsys,&attributes)==0 && opsys==ZIP_OPSYS_UNIX && ((attributes>>16)&0170000)==0120000)throw std::runtime_error("VPK links are not allowed");
            if(path.back()=='/'){pkgi_mkdirs(path.c_str());continue;}
            pkgi_mkdirs(path.substr(0,path.find_last_of('/')).c_str());zip_file_t*in=zip_fopen_index(z,i,0);if(!in)throw std::runtime_error("Cannot read VPK entry");
            void*file=nullptr;try{file=pkgi_create(path);uint64_t written=0;zip_int64_t n;
                while((n=zip_fread(in,buffer,sizeof(buffer)))>0){if(cancel_job)throw std::runtime_error("Download canceled");pkgi_write(file,buffer,(uint32_t)n);written+=n;}
                if(n<0 || written!=st.size)throw std::runtime_error("VPK CRC or length mismatch");
                pkgi_close(file);zip_fclose(in);
            }catch(...){if(file)pkgi_close(file);zip_fclose(in);throw;}
        }
        zip_discard(z);
    }catch(...){zip_discard(z);throw;}
}
static void find_vpk(const std::string&root,std::vector<std::string>&found,unsigned depth=0){
    if(depth>12)throw std::runtime_error("ZIP nesting too deep");
    for(const auto&name:pkgi_list_dir_contents(root)){
        auto path=root+"/"+name;if(pkgi_get_inode_type(path)==InodeType::Directory)find_vpk(path,found,depth+1);
        else if(name.size()>4 && name.substr(name.size()-4)==".vpk")found.push_back(path);
    }
}
static void download_vpk(const CatalogPackage&p,const std::string&root){
    pkgi_delete_dir(root);pkgi_mkdirs(root.c_str());const auto archive=root+".vpk";
    download_file(p,archive);extract_archive(archive,root);pkgi_rm(archive.c_str());
    if(!pkgi_file_exists(root+"/sce_sys/param.sfo") && p.format==CATALOG_LINK_APP_ZIP){
        std::vector<std::string> found;find_vpk(root,found);if(found.size()!=1)throw std::runtime_error("O ZIP deve conter um unico VPK.");
        pkgi_rename(found.front(),archive);pkgi_delete_dir(root);pkgi_mkdirs(root.c_str());extract_archive(archive,root);pkgi_rm(archive.c_str());
    }homebrew_head(root,p);
}
#include "store_merge.inc"
static void merge_psp_dlc(const std::string&root,const std::string&dest){
    std::vector<MergeFile> files;collect_merge(root,"",files);const auto backup=root+"/__VitaStation_previous";
    try{
        for(auto&file:files){
            const auto from=root+"/"+file.relative,to=dest+"/"+file.relative,previous=backup+"/"+file.relative;
            auto type=pkgi_get_inode_type(to);if(type==InodeType::Directory)throw std::runtime_error("PSP DLC target is a directory");
            pkgi_mkdirs(to.substr(0,to.find_last_of('/')).c_str());
            if(type==InodeType::File){pkgi_mkdirs(previous.substr(0,previous.find_last_of('/')).c_str());pkgi_rename(to,previous);file.old=true;}
            pkgi_rename(from,to);file.moved=true;
        }
    }catch(...){
        bool restored=true;
        for(auto i=files.rbegin();i!=files.rend();++i){try{
            if(i->moved)pkgi_rename(dest+"/"+i->relative,root+"/"+i->relative);
            if(i->old)pkgi_rename(backup+"/"+i->relative,dest+"/"+i->relative);
        }catch(...){restored=false;}}
        if(!restored)throw std::runtime_error("PSP DLC recovery incomplete; previous files preserved in staging");
        throw;
    }
    pkgi_delete_dir(root);
}
static void install_package(const CatalogPackage&p,const std::string&root){
    phase(STORE_INSTALLING,"A instalar automaticamente...");
    /* Installation cannot be interrupted halfway through a promoter transaction. */
    if(p.vpk){
        const std::string shallow="ux0:data/vst_vpk";
        if(pkgi_get_inode_type(shallow)!=InodeType::NotExist)pkgi_rename(shallow,"ux0:data/VitaStation/pkg-staging/recovery-"+std::to_string(sceKernelGetProcessTimeWide()));
        pkgi_rename(root,shallow);
        int result=scePromoterUtilityPromotePkg(shallow.c_str(),1);
        if(pkgi_get_inode_type(shallow)!=InodeType::NotExist)pkgi_rename(shallow,root);
        check(result,"Install homebrew");return;
    }
    if(p.type==CATALOG_PSP || p.type==CATALOG_PS1 || p.type==CATALOG_PSP_DLCS){
        std::string id=p.title_id;
        auto pbp=root+"/EBOOT.PBP";
        if(p.type!=CATALOG_PSP_DLCS){
            void*in=pkgi_openrw(pbp.c_str());if(!in)throw std::runtime_error("Missing EBOOT.PBP");
            try{uint8_t header[40];if(pkgi_read(in,header,40)!=40 || get32le(header)!=0x50425000)throw std::runtime_error("Invalid PBP header");
                uint32_t a=get32le(header+8),b=get32le(header+12);if(a<40 || b<a || b-a>1024*1024)throw std::runtime_error("Invalid PBP metadata");
                pkgi_seek(in,a);std::vector<uint8_t>sfo(b-a);if(pkgi_read(in,sfo.data(),sfo.size())!=(int)sfo.size())throw std::runtime_error("Truncated PBP metadata");auto disc=sfo_string(sfo,"DISC_ID");if(disc.size()==9 && std::all_of(disc.begin(),disc.end(),[](unsigned char c){return std::isalnum(c);}))id=disc;pkgi_close(in);
            }catch(...){pkgi_close(in);throw;}
        }
        const auto dest=fmt::format("ux0:pspemu/PSP/GAME/{}",id);pkgi_mkdirs("ux0:pspemu/PSP/GAME");
        pkgi_delete_dir(root+"/sce_sys");
        if(p.type==CATALOG_PSP_DLCS && pkgi_file_exists(dest)){merge_psp_dlc(root,dest);return;}
        pkgi_rename(root,dest);return;
    }
    if(p.type==CATALOG_PSM){
        const char*base="ux0:temp/game";pkgi_mkdirs(base);auto dest=fmt::format("{}/{}",base,p.title_id);pkgi_rename(root,dest);
        ScePromoterUtilityImportParams args{};snprintf(args.path,sizeof(args.path),"%s",base);snprintf(args.titleid,sizeof(args.titleid),"%s",p.title_id);args.type=SCE_PKG_TYPE_PSM;args.attribute=1;check(scePromoterUtilityPromoteImport(&args),"Install PSM");return;
    }
    check(scePromoterUtilityPromotePkgWithRif(root.c_str(),1),"Install package");
}
#include "store_alive.inc"
static void run_job(const CatalogPackage&p){
    init_network();if(p.size_bytes)free_space(p.size_bytes*2);
    const std::string root=fmt::format("ux0:data/VitaStation/pkg-staging/{}",p.type==CATALOG_HOMEBREW?p.cache_key:(p.vpk?p.title_id:p.content_id));
    if(p.type==CATALOG_HOMEBREW && !p.vpk){install_homebrew_payload(p,root);pkgi_delete_dir(root);return;}
    if(p.vpk)download_vpk(p,root);
    else{
        init_legacy_http();
        uint8_t rif[PKGI_PSM_RIF_SIZE]{};const uint8_t*license=nullptr;
        if(p.zrif[0] && strcmp(p.zrif,"MISSING")){char error[256];if(!pkgi_zrif_decode(p.zrif,rif,error,sizeof(error)))throw std::runtime_error(error);license=rif;}
        auto digest=pkgi_hexbytes(p.digest,32);Download d(std::make_unique<VitaHttp>());
        d.update_progress_cb=progress_bytes;d.update_status=[](const std::string&){phase(STORE_DOWNLOADING,"A descarregar e verificar o pacote...");};d.is_canceled=[](){return cancel_job.load();};
        if(!d.pkgi_download("ux0:",p.content_id,p.url,license,p.digest[0]?digest.data():nullptr))throw std::runtime_error("Package download failed");
    }
    if(cancel_job)throw std::runtime_error("Download canceled");
    init_services();install_package(p,root);
    pkgi_delete_dir(root);pkgi_rm((root+".resume").c_str());
}
struct MemoryResponse {std::vector<uint8_t> bytes;size_t limit;};
static size_t memory_write(char*bytes,size_t size,size_t count,void*context){
    auto*data=static_cast<MemoryResponse*>(context);
    if(size && count>SIZE_MAX/size)return 0;
    size_t length=size*count;
    if(length>data->limit-data->bytes.size() || stop_worker)return 0;
    try{data->bytes.insert(data->bytes.end(),bytes,bytes+length);}catch(...){return 0;}return length;
}
static CURL* https_client(const std::string&url){
    init_network();CURL*h=curl_easy_init();if(!h)throw std::runtime_error("Nao foi possivel iniciar HTTPS.");
    curl_easy_setopt(h,CURLOPT_URL,url.c_str());
    curl_easy_setopt(h,CURLOPT_CAINFO,nullptr);curl_easy_setopt(h,CURLOPT_CAPATH,nullptr);
    curl_easy_setopt(h,CURLOPT_CAINFO_BLOB,nullptr);
    curl_easy_setopt(h,CURLOPT_CA_CACHE_TIMEOUT,0L);
    CURLcode configured=curl_easy_setopt(h,CURLOPT_SSL_CTX_FUNCTION,attach_ca_store);
    if(configured!=CURLE_OK){curl_easy_cleanup(h);throw std::runtime_error("OpenSSL: adaptador de certificados indisponivel.");}
    curl_easy_setopt(h,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(h,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(h,CURLOPT_CONNECTTIMEOUT,10L);curl_easy_setopt(h,CURLOPT_LOW_SPEED_LIMIT,128L);curl_easy_setopt(h,CURLOPT_LOW_SPEED_TIME,30L);
    curl_easy_setopt(h,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(h,CURLOPT_FAILONERROR,1L);curl_easy_setopt(h,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(h,CURLOPT_MAXREDIRS,5L);
    curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"http,https");curl_easy_setopt(h,CURLOPT_REDIR_PROTOCOLS_STR,"http,https");
    curl_easy_setopt(h,CURLOPT_USERAGENT,"VitaStation/" VS_VERSION);return h;
}
static std::vector<uint8_t> https_bytes(const std::string&url,size_t limit){
    CURL*h=https_client(url);if(url.find("https://api.github.com/")==0 || url.find("https://raw.githubusercontent.com/")==0 || url.find("https://github.com/")==0){curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(h,CURLOPT_REDIR_PROTOCOLS_STR,"https");}char detail[CURL_ERROR_SIZE]{};curl_easy_setopt(h,CURLOPT_ERRORBUFFER,detail);MemoryResponse data{{},limit};curl_easy_setopt(h,CURLOPT_TIMEOUT,45L);
    curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,memory_write);curl_easy_setopt(h,CURLOPT_WRITEDATA,&data);
    auto code=curl_easy_perform(h);curl_easy_cleanup(h);
    if(code!=CURLE_OK)throw std::runtime_error(transfer_error(code,detail));
    return std::move(data.bytes);
}
#include "update_covers.inc"
static unsigned submit_system_package(const CatalogPackage&p);
#include "game_support.inc"
static void run_update(bool download){
    if(!download){
        auto bytes=https_bytes("https://api.github.com/repos/CarlosTuga/VitaStation/releases?per_page=100",2*1024*1024);
        ensure_update_covers(bytes);
        UpdateStatus found{};if(!update_select((const char*)bytes.data(),bytes.size(),&found))throw std::runtime_error("Resposta de atualizacoes invalida.");
        lock();update_status=found;unlock();return;
    }
    lock();UpdateStatus selected=update_status;unlock();
    /* Bound memory and require TLS also after GitHub's asset redirect. */
    CURL*h=https_client(selected.url);curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(h,CURLOPT_REDIR_PROTOCOLS_STR,"https");
    MemoryResponse data{{},64*1024*1024};curl_easy_setopt(h,CURLOPT_TIMEOUT,600L);curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,memory_write);curl_easy_setopt(h,CURLOPT_WRITEDATA,&data);
    auto code=curl_easy_perform(h);curl_easy_cleanup(h);if(code!=CURLE_OK)throw std::runtime_error(transfer_error(code,""));
    if(data.bytes.size()!=selected.size)throw std::runtime_error("Tamanho do VPK incorreto.");
    unsigned char sum[32];SHA256(data.bytes.data(),data.bytes.size(),sum);char digest[65];for(unsigned i=0;i<32;i++)snprintf(digest+i*2,3,"%02x",sum[i]);
    if(strcasecmp(digest,selected.digest))throw std::runtime_error("SHA-256 da atualizacao incorreto.");
    const std::string dir="ux0:data/VitaStation/updates",tmp=dir+"/VitaStation.part",dest=dir+"/VitaStation.vpk";pkgi_mkdirs(dir.c_str());pkgi_save(tmp,data.bytes.data(),data.bytes.size());pkgi_rm(dest.c_str());pkgi_rename(tmp,dest);
    lock();update_status.phase=3;snprintf(update_status.message,sizeof(update_status.message),"VPK verificado: ux0:data/VitaStation/updates/VitaStation.vpk. Instala no VitaShell.");unlock();
}
extern "C" void store_update_status(UpdateStatus*out){if(!out)return;if(lock_id<0){*out={};snprintf(out->message,sizeof(out->message),"Servico de atualizacoes indisponivel.");return;}lock();*out=update_status;if(!out->message[0])snprintf(out->message,sizeof(out->message),"X: procurar atualizacoes no GitHub.");unlock();}
extern "C" void store_update_request(){
    if(lock_id<0 || cover_thread_id<0)return;
    lock();if(update_status.phase==1){unlock();return;}if(progress.active || count || game_status.busy){snprintf(update_status.message,sizeof(update_status.message),"Aguarda a fila local antes de atualizar.");unlock();return;}update_busy=true;update_download=update_status.phase==2;update_pending=true;update_status.phase=1;snprintf(update_status.message,sizeof(update_status.message),update_download?"A descarregar e verificar o VPK...":"A procurar atualizacoes...");unlock();sceKernelSignalSema(cover_wake_id,1);
}
static void refresh_homebrew(){
    auto bytes=https_bytes("https://raw.githubusercontent.com/VegettoSan/PSVitaAlive/main/catalog.json",16*1024*1024);
    const std::string root="ux0:data/VitaStation/alive",temporary=root+"/incoming";pkgi_mkdirs(temporary.c_str());
    pkgi_save(temporary+"/catalog.json",bytes.data(),bytes.size());
    StoreCatalog catalog;catalog_init(&catalog);bool valid=catalog_load_root(&catalog,temporary.c_str(),CATALOG_HOMEBREW) && catalog.count>0;catalog_clear(&catalog);
    if(!valid)throw std::runtime_error("O novo catalogo Homebrew e invalido; anterior preservado.");
    auto target=root+"/catalog.json",backup=root+"/catalog.previous.json";bool had=pkgi_file_exists(target);
    if(had){pkgi_rm(backup.c_str());pkgi_rename(target,backup);}
    try{pkgi_rename(temporary+"/catalog.json",target);}catch(...){if(had)pkgi_rename(backup,target);throw;}
    lock();++progress.homebrew_revision;progress.homebrew_error[0]=0;unlock();
}
static void fetch_cover(const CoverRequest&request){
    if(stop_worker)return;
    init_network();
    const char*folder=request.type==CATALOG_PSP || request.type==CATALOG_PSP_DLCS?"PSP":request.type==CATALOG_PS1?"PS1":"PSVita";
    const auto url=request.url[0]?std::string(request.url):fmt::format("https://raw.githubusercontent.com/Andiweli/HexFlow-Covers/main/Covers/{}/{}.png",folder,request.id);
    auto image=https_bytes(url,2*1024*1024);
    if(image.size()<24)throw std::runtime_error("A imagem da caratula esta incompleta.");
    if(memcmp(image.data(),"\x89PNG\r\n\x1a\n",8) || memcmp(image.data()+12,"IHDR",4))return;
    uint32_t w=get32be(image.data()+16),h=get32be(image.data()+20);if(!w || !h || w>1920 || h>1088)return;
    pkgi_mkdirs("ux0:data/VitaStation/covers");auto path=fmt::format("ux0:data/VitaStation/covers/{}.png",request.id);
    if(pkgi_file_exists(path)){
        auto existing=pkgi_load(path);
        if(existing.size()>=24 && !memcmp(existing.data(),"\x89PNG\r\n\x1a\n",8))return;
        pkgi_rm(path.c_str()); /* Regenerable cache owned by this application. */
    }
    auto temporary=path+".tmp";
    pkgi_save(temporary,image.data(),image.size());pkgi_rename(temporary,path);
    lock();++progress.covers_revision;unlock();
}
static bool install_dlc(const CatalogPackage*p,void*){
    lock();snprintf(progress.title,sizeof(progress.title),"DLC | %.180s",p->name);progress.downloaded=progress.total=0;unlock();
    try{run_job(*p);return true;}catch(const std::exception&e){vs_log("dlc",e.what(),-1);return false;}
}
static bool dlc_canceled(void*){return cancel_job.load() || stop_worker.load();}
static std::string install_game_bundle(const CatalogPackage&p){
    run_job(p);
    if(p.type==CATALOG_HOMEBREW && p.format==CATALOG_LINK_PLUGIN)return "Plugin configurado. Reinicia a consola para aplicar.";
    if(!p.auto_dlcs || p.type!=CATALOG_GAMES || p.vpk)return "Instalacao concluida.";
    StoreCatalog catalog;catalog_init(&catalog);
    phase(STORE_DOWNLOADING,"Jogo instalado. A procurar DLCs da mesma ID...");
    if(!catalog_load_matching_auto(&catalog,CATALOG_DLCS,p.title_id)){catalog_clear(&catalog);return "Jogo instalado; catalogo de DLCs indisponivel.";}
    DLCPlanResult result=dlc_plan_run(&p,&catalog,install_dlc,dlc_canceled,nullptr);catalog_clear(&catalog);
    lock();progress.failed+=(unsigned)result.failed;unlock();
    return fmt::format("Jogo instalado. DLCs: {} instalados, {} falhas, {} indisponiveis{}.",result.installed,result.failed,result.unavailable,result.canceled?", cancelado":"");
}
static int cover_worker(SceSize,void*){
    while(!stop_worker){
        sceKernelWaitSema(cover_wake_id,1,nullptr);if(stop_worker)break;
        lock();bool updating=update_pending,download=update_download;update_pending=false;unlock();
        if(updating){try{run_update(download);}catch(const std::exception&e){lock();update_status.phase=5;snprintf(update_status.message,sizeof(update_status.message),"%s",e.what());unlock();}lock();update_busy=false;unlock();}
        lock();bool game=game_pending;AppItem app=game_request;int action=game_action,selection=game_selection;game_pending=false;char background[32];snprintf(background,sizeof(background),"%s",background_request);background_request[0]=0;unlock();
        if(game){try{run_game_request(app,action,selection);}catch(const std::exception&e){game_message(e.what());}lock();game_status.busy=false;unlock();}
        if(background[0]){try{fetch_game_background(background);}catch(const std::exception&e){vs_log("background",e.what(),-1);}}
        lock();bool refresh=homebrew_refresh_pending;homebrew_refresh_pending=false;unlock();
        if(refresh){try{refresh_homebrew();}catch(const std::exception&e){vs_log("homebrew-catalog",e.what(),-1);lock();snprintf(progress.homebrew_error,sizeof(progress.homebrew_error),"%s",e.what());unlock();}}
        lock();if(!cover_count){unlock();continue;}
        CoverRequest request=cover_queue[cover_first];cover_first=(cover_first+1)%16;--cover_count;unlock();
        try{fetch_cover(request);}catch(const std::exception&e){vs_log("covers",e.what(),-1);lock();++progress.covers_failed;snprintf(progress.cover_error,sizeof(progress.cover_error),"%s",e.what());unlock();}
    }return sceKernelExitThread(0);
}
static int worker(SceSize,void*){
    while(!stop_worker){sceKernelWaitSema(wake_id,1,nullptr);if(stop_worker)break;
        CatalogPackage p{};lock();if(!count){unlock();continue;}
        p=queue[first];first=(first+1)%8;--count;progress.queued=count;progress.active=true;progress.phase=STORE_DOWNLOADING;progress.downloaded=progress.total=0;snprintf(progress.title,sizeof(progress.title),"%s",p.name);snprintf(active_content_id,sizeof(active_content_id),"%s",p.content_id);snprintf(active_title_id,sizeof(active_title_id),"%s",p.title_id);snprintf(active_url,sizeof(active_url),"%s",p.url);cancel_job=false;unlock();
        try{auto message=install_game_bundle(p);installation_mark_dirty();lock();++progress.completed;progress.phase=STORE_COMPLETE;snprintf(progress.message,sizeof(progress.message),"%s",message.c_str());unlock();}
        catch(const std::exception&e){lock();progress.phase=cancel_job?STORE_CANCELED:STORE_FAILED;if(!cancel_job)++progress.failed;snprintf(progress.message,sizeof(progress.message),"%s",e.what());unlock();}
        lock();progress.active=false;++progress.revision;unlock();
    }return sceKernelExitThread(0);
}
extern "C" bool store_start(){
    offline_covers=true;progress.offline_covers=true; /* This beta uses permanent local artwork only. */
    lock_id=sceKernelCreateMutex("vs-store-lock",0,0,nullptr);if(lock_id<0)return false;
    wake_id=sceKernelCreateSema("vs-store-wake",0,0,64,nullptr);if(wake_id<0){sceKernelDeleteMutex(lock_id);lock_id=-1;return false;}
    thread_id=sceKernelCreateThread("vs-store",worker,0x10000100,256*1024,0,0,nullptr);
    if(thread_id>=0 && sceKernelStartThread(thread_id,0,nullptr)>=0){
        cover_wake_id=sceKernelCreateSema("vs-covers-wake",0,0,32,nullptr);
        if(cover_wake_id>=0){
            cover_thread_id=sceKernelCreateThread("vs-covers",cover_worker,0x10000100,256*1024,0,0,nullptr);
            if(cover_thread_id>=0 && sceKernelStartThread(cover_thread_id,0,nullptr)<0){sceKernelDeleteThread(cover_thread_id);cover_thread_id=-1;}
        }
        if(cover_thread_id<0){progress.covers_failed=1;snprintf(progress.cover_error,sizeof(progress.cover_error),"Nao foi possivel iniciar o carregamento de caratulas.");}
        return true;
    }
    if(thread_id>=0)sceKernelDeleteThread(thread_id);
    sceKernelDeleteSema(wake_id);sceKernelDeleteMutex(lock_id);lock_id=wake_id=thread_id=-1;return false;
}
static unsigned submit_system_package(const CatalogPackage&p){
    int type;
    switch(p.type){
        case CATALOG_GAMES:case CATALOG_DEMOS:type=BgdlTypeGame;break;
        case CATALOG_DLCS:type=BgdlTypeDlc;break;
        case CATALOG_THEMES:type=BgdlTypeTheme;break;
        case CATALOG_PSM:type=BgdlTypePsm;break;
        case CATALOG_PSP:case CATALOG_PS1:case CATALOG_PSP_DLCS:type=BgdlTypePsp;break;
        default:throw std::runtime_error("Este formato nao e um PKG do sistema.");
    }
    if(type==BgdlTypePsm){SceSysmoduleOpt opt{};if(_vshKernelSearchModuleByName("NoPsmDrm",&opt)<0)throw std::runtime_error("PSM requer NoPsmDrm carregado e o runtime PSM instalado.");}
    std::vector<uint8_t> license;
    if(type==BgdlTypePsp){
        SceSysmoduleOpt opt{};
        if(_vshKernelSearchModuleByName("NoPspEmuDrm_kern",&opt)<0)throw std::runtime_error("PSP/PS1 no LiveArea requer NoPspEmuDrm carregado. Reinicia apos instalar o plugin.");
        if(!p.content_id[0] || strlen(p.content_id)>=48)throw std::runtime_error("O catalogo nao forneceu Content ID para este pacote PSP/PS1.");
        SceNpDrmLicense rif{};rif.account_id=0x0123456789ABCDEFLL;memset(rif.ecdsa_signature,0xff,sizeof(rif.ecdsa_signature));memcpy(rif.content_id,p.content_id,strlen(p.content_id)+1);
        const auto*bytes=reinterpret_cast<const uint8_t*>(&rif);license.assign(bytes,bytes+PKGI_PSP_RIF_SIZE);
    }else if(p.zrif[0] && strcmp(p.zrif,"MISSING")){
        license.resize(PKGI_PSM_RIF_SIZE);char error[256];if(!pkgi_zrif_decode(p.zrif,license.data(),error,sizeof(error)))throw std::runtime_error(error);
        if(type!=BgdlTypePsm)license.resize(PKGI_RIF_SIZE);
    }
    pkgi_mkdirs("ux0:data/VitaStation/bgdl");const std::string icon="ux0:data/VitaStation/bgdl/icon0.png";
    if(!pkgi_file_exists(icon)){auto image=pkgi_load("app0:sce_sys/icon0.png");pkgi_save(icon,image.data(),image.size());}
    unsigned id=vs_start_bgdl(type,p.name,p.url,license,icon);
    lock();++progress.system_submitted;unlock();return id;
}
struct SystemDlcQueue {unsigned accepted=0;};
static bool submit_system_dlc(const CatalogPackage*p,void*context){
    try{submit_system_package(*p);++static_cast<SystemDlcQueue*>(context)->accepted;return true;}catch(const std::exception&e){vs_log("bgdl-dlc",e.what(),-1);return false;}
}
static bool enqueue_system_bundle(const CatalogPackage&p,char*error,size_t size){
    try{
        unsigned id=submit_system_package(p);std::string message=fmt::format("LiveArea: tarefa {} aceite. Podes abrir outro jogo.",id);
        if(p.auto_dlcs && p.type==CATALOG_GAMES){
            StoreCatalog catalog;catalog_init(&catalog);SystemDlcQueue queue;
            if(catalog_load_matching_auto(&catalog,CATALOG_DLCS,p.title_id)){
                DLCPlanResult result=dlc_plan_run(&p,&catalog,submit_system_dlc,nullptr,&queue);
                message=fmt::format("LiveArea: jogo + {} DLCs em fila; {} falhas, {} indisponiveis.",result.installed,result.failed,result.unavailable);
            }else message+=" Catalogo de DLCs indisponivel.";
            catalog_clear(&catalog);
        }
        lock();progress.phase=STORE_SYSTEM_QUEUED;++progress.revision;snprintf(progress.message,sizeof(progress.message),"%s",message.c_str());unlock();return true;
    }catch(const std::exception&e){vs_log("bgdl",e.what(),-1);if(error && size)snprintf(error,size,"%s",e.what());return false;}
}
extern "C" bool store_enqueue(const CatalogPackage*p,char*error,size_t size){
    const char*message=nullptr;
    if(thread_id<0)message="Motor de instalacao indisponivel.";
    else if(!p || (!p->title_id[0] && p->type!=CATALOG_HOMEBREW) || !strcmp(p->title_id,"VTST00001"))message="Nao e possivel substituir o launcher em execucao.";
    else{
        if(!p->vpk && p->type!=CATALOG_HOMEBREW)return enqueue_system_bundle(*p,error,size);
        lock();
        if(update_busy || game_status.busy)message="Atualizacao ativa; aguarda a conclusao.";
        else if(count>=8)message="A fila de descargas esta cheia.";
        else{
            for(unsigned i=0;i<count;i++)if(!strcmp(queue[(first+i)%8].url,p->url))message="O item ja esta na fila.";
            if(progress.active && !strcmp(active_url,p->url))message="Este item ja esta a ser instalado.";
            if(!message){queue[(first+count)%8]=*p;++count;progress.queued=count;}
        }unlock();
        if(!message){sceKernelSignalSema(wake_id,1);return true;}
    }
    if(error && size)snprintf(error,size,"%s",message);
    return false;
}
extern "C" void store_progress(StoreProgress*p){if(!p)return;if(lock_id<0){*p={};return;}lock();*p=progress;unlock();}
extern "C" bool store_busy(){StoreProgress p{};store_progress(&p);UpdateStatus u{};store_update_status(&u);GameSupportStatus g{};store_game_status(&g);return p.active || p.queued || u.phase==1 || g.busy;}
extern "C" void store_retry_covers(){if(lock_id<0)return;lock();memset(cover_history,0,sizeof(cover_history));cover_history_next=0;progress.covers_failed=0;progress.cover_error[0]=0;unlock();}
extern "C" void store_request_image(CatalogType type,const char*id,const char*url){
    if(offline_covers)return; /* Permanent library: no network request, deletion or expiry. */
    if(cover_thread_id<0 || !id || !*id || strlen(id)>=64 || (unsigned)type>=CATALOG_TYPE_COUNT || (url && strlen(url)>=1024))return;
    for(const char*c=id;*c;c++)if(!std::isalnum((unsigned char)*c) && *c!='_')return;
    lock();for(const auto&known:cover_history)if(!strcmp(known,id)){unlock();return;}
    if(cover_count>=16){unlock();return;}
    CoverRequest request{};request.type=type;snprintf(request.id,sizeof(request.id),"%s",id);if(url)snprintf(request.url,sizeof(request.url),"%s",url);
    cover_queue[(cover_first+cover_count)%16]=request;++cover_count;
    snprintf(cover_history[cover_history_next++%512],64,"%s",id);unlock();sceKernelSignalSema(cover_wake_id,1);
}
extern "C" void store_request_cover(CatalogType type,const char*id){if(id && strlen(id)==9)store_request_image(type,id,nullptr);}
extern "C" void store_refresh_homebrew(bool force){
    if(offline_covers)return; /* Bundled/local catalog and installed art work without HTTPS. */
    if(cover_thread_id<0)return;
    lock();if(homebrew_refresh_pending || (homebrew_refresh_started && !force)){unlock();return;}
    homebrew_refresh_started=true;homebrew_refresh_pending=true;unlock();sceKernelSignalSema(cover_wake_id,1);
}
extern "C" void store_cancel(){cancel_job=true;if(lock_id>=0){lock();count=0;progress.queued=0;unlock();}}
extern "C" void store_shutdown(){if(thread_id<0)return;stop_worker=true;
    if(cover_thread_id>=0){sceKernelSignalSema(cover_wake_id,1);sceKernelWaitThreadEnd(cover_thread_id,nullptr,nullptr);sceKernelDeleteThread(cover_thread_id);cover_thread_id=-1;}
    if(cover_wake_id>=0){sceKernelDeleteSema(cover_wake_id);cover_wake_id=-1;}
    sceKernelSignalSema(wake_id,1);sceKernelWaitThreadEnd(thread_id,nullptr,nullptr);sceKernelDeleteThread(thread_id);
    sceKernelDeleteSema(wake_id);sceKernelDeleteMutex(lock_id);thread_id=lock_id=wake_id=-1;}
