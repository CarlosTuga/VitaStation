
#define ROOT "ux0:data/VitaStation/covers"
#include "cover_commit.inc"
static void update_message(const char*message){lock();snprintf(update_status.message,sizeof(update_status.message),"%s",message);unlock();}
static int update_transfer_progress(void*,curl_off_t total,curl_off_t done,curl_off_t,curl_off_t){
    char status[160];snprintf(status,sizeof(status),"A descarregar: %llu / %llu MiB. Mantem o launcher aberto.",(unsigned long long)done/1048576,(unsigned long long)total/1048576);update_message(status);sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);return stop_worker.load();
}
static void update_download_disk(const UpdateStatus&asset,const std::string&path){
    free_space(asset.size);CURL*h=https_client(asset.url);FileTransfer t{};t.limit=asset.size;sha256_init(&t.sha);
    curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(h,CURLOPT_REDIR_PROTOCOLS_STR,"https");
    curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,file_write);curl_easy_setopt(h,CURLOPT_WRITEDATA,&t);curl_easy_setopt(h,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(h,CURLOPT_XFERINFOFUNCTION,update_transfer_progress);
    char error[CURL_ERROR_SIZE]{};curl_easy_setopt(h,CURLOPT_ERRORBUFFER,error);
    try{t.file=pkgi_create(path);}catch(...){curl_easy_cleanup(h);throw;}
    auto code=curl_easy_perform(h);curl_easy_cleanup(h);pkgi_close(t.file);
    if(code!=CURLE_OK || t.failed || t.done!=asset.size){pkgi_rm(path.c_str());throw std::runtime_error(code==CURLE_OK?"Tamanho do pacote incorreto.":transfer_error(code,error));}
    uint8_t sum[32];sha256_finish(&t.sha,sum);
    for(unsigned i=0;i<32;i++)if(sum[i]!=(hexvalue(asset.digest[i*2])*16+hexvalue(asset.digest[i*2+1]))){pkgi_rm(path.c_str());throw std::runtime_error("SHA-256 do pacote incorreto.");}
}
static bool installed_covers(){
    if(cover_pack_available())return true;
    if(pkgi_get_inode_type(ROOT)!=InodeType::Directory)return false;
    for(const auto&name:pkgi_list_dir_contents(ROOT)){
        if(!safe_relative(name) || name.find('/')!=std::string::npos)continue;
        if(name.size()<5 || (name.substr(name.size()-4)!=".png" && name.substr(name.size()-4)!=".jpg"))continue;
        auto path=std::string(ROOT)+"/"+name;SceUID fd=sceIoOpen(path.c_str(),SCE_O_RDONLY,0);if(fd<0)continue;
        unsigned char header[8]{};int n=sceIoRead(fd,header,8);sceIoClose(fd);
        if(n==8 && (!memcmp(header,"\x89PNG\r\n\x1a\n",8) || (header[0]==0xff && header[1]==0xd8 && header[2]==0xff)))return true;
    }return false;
}
#include "cover_install.inc"
static void ensure_update_covers(const std::vector<uint8_t>&releases){
    update_message("A verificar caratulas instaladas...");if(installed_covers())return;
    UpdateStatus asset{};if(!update_select_covers((const char*)releases.data(),releases.size(),&asset)){
        auto release=https_bytes("https://api.github.com/repos/CarlosTuga/VitaStation/releases/tags/v1.6.0-beta",2*1024*1024);
        std::string wrapped="["+std::string(release.begin(),release.end())+"]";
        if(!update_select_covers(wrapped.data(),wrapped.size(),&asset))throw std::runtime_error("Pacote Caratulas-de-loja-1.04.vpk indisponivel no GitHub.");
    }
    pkgi_mkdirs("ux0:data/VitaStation/updates");const std::string archive="ux0:data/VitaStation/updates/Caratulas-de-loja-1.04.vpk.part";
    free_space((uint64_t)asset.size+300*1024*1024);
    try{cancel_job=false;update_download_disk(asset,archive);install_cover_archive(archive);pkgi_rm(archive.c_str());}
    catch(...){pkgi_rm(archive.c_str());throw;}
}
#undef ROOT
