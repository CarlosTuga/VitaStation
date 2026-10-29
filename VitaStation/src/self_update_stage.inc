#include "launch_handoff.h"
static unsigned app_version_number(const std::string&s){unsigned a=0,b=0;if(sscanf(s.c_str(),"%u.%u",&a,&b)!=2 || a>99 || b>999)return 0;return a*1000+b;}
static void stage_self_update(const UpdateStatus&selected){
 char error[256];if(!safety_backup_ensure(error,sizeof(error)))throw std::runtime_error(error);
 const std::string archive="ux0:data/VitaStation/updates/VitaStation.vpk.part",root=VS_UPDATE_ROOT,rollback=VS_ROLLBACK_ROOT;
 pkgi_mkdirs("ux0:data/VitaStation/updates");pkgi_mkdirs("ux0:data/VitaStation/pkg-staging");
 cancel_job=false;update_download_disk(selected,archive);
 try{
    auto*z=open_vita_zip(archive);uint64_t expanded=0;auto entries=zip_get_num_entries(z,0);
    if(entries<=0 || entries>8192){zip_discard(z);throw std::runtime_error("Atualizacao com numero de ficheiros invalido.");}
    for(zip_int64_t i=0;i<entries;i++){zip_stat_t st{};if(zip_stat_index(z,i,0,&st)<0 || st.size>512*1024*1024-expanded){zip_discard(z);throw std::runtime_error("Atualizacao excede 512 MiB.");}expanded+=st.size;}zip_discard(z);
    free_space(expanded*2+128*1024*1024);
    update_message("A preparar e verificar todos os recursos da nova versao...");
    pkgi_delete_dir(root);pkgi_mkdirs(root.c_str());extract_archive(archive,root);
    auto metadata=pkgi_load(root+"/sce_sys/param.sfo");auto id=sfo_string(metadata,"TITLE_ID"),version=sfo_string(metadata,"APP_VER");
    auto current=sfo_string(pkgi_load("app0:sce_sys/param.sfo"),"APP_VER");
    if(id!="VTST00001" || app_version_number(version)<=app_version_number(current))throw std::runtime_error("O VPK nao e uma nova versao do VitaStation.");
    if(!pkgi_file_exists(root+"/assets/updater/eboot.bin") || !pkgi_file_exists(root+"/assets/updater/param.sfo"))throw std::runtime_error("Atualizacao sem recursos do atualizador automatico.");
    CatalogPackage app{};app.vpk=true;snprintf(app.title_id,sizeof(app.title_id),"VTST00001");homebrew_head(root,app,true);
    if(!self_update_prepare(root.c_str(),VS_UPDATE_TICKET,error,sizeof(error)))throw std::runtime_error(error);
    update_message("A guardar uma copia da versao instalada para recuperacao...");
    pkgi_delete_dir(rollback);
    if(!self_update_copy("app0:",rollback.c_str(),error,sizeof(error)))throw std::runtime_error(error);
    homebrew_head(rollback,app,true);
    if(!self_update_prepare(rollback.c_str(),VS_ROLLBACK_TICKET,error,sizeof(error)))throw std::runtime_error(error);
    const std::string helper="ux0:data/VitaStation/pkg-staging/updater-helper";pkgi_delete_dir(helper);pkgi_mkdirs((helper+"/sce_sys").c_str());
    for(const auto&pair:std::vector<std::pair<const char*,const char*>>{{"app0:assets/updater/eboot.bin","/eboot.bin"},{"app0:assets/updater/param.sfo","/sce_sys/param.sfo"},{"app0:sce_sys/icon0.png","/sce_sys/icon0.png"}}){auto data=pkgi_load(pair.first);pkgi_save(helper+pair.second,data.data(),data.size());}
    CatalogPackage tool{};tool.vpk=true;snprintf(tool.title_id,sizeof(tool.title_id),VS_UPDATER_ID);homebrew_head(helper,tool);init_services();install_package(tool,helper);
    pkgi_rm(archive.c_str());
    lock();update_status=selected;update_status.phase=3;snprintf(update_status.message,sizeof(update_status.message),"Atualizacao verificada. A instalar automaticamente...");unlock();
 }catch(...){pkgi_rm(archive.c_str());throw;}
}
extern "C" bool store_update_install(char*error,size_t capacity){
 lock();bool ready=update_status.phase==3 && !progress.active && !count && !game_status.busy;
 if(ready){update_status.phase=6;update_busy=true;}unlock();
 if(!ready){snprintf(error,capacity,"Aguarda a preparacao da atualizacao.");return false;}
 store_shutdown();
 int code=shell_handoff_request("VTST00001",VS_UPDATER_ID)?0:-0x7001;
 if(code>=0)sceKernelExitProcess(0);
 snprintf(error,capacity,"Nao foi possivel abrir o atualizador: 0x%08X. Reabre o VitaStation.",(unsigned)code);return false;
}
static void cleanup_updater(){
 init_services();int code=scePromoterUtilityDeletePkg(VS_UPDATER_ID);if(code<0)throw std::runtime_error("Atualizacao instalada; auxiliar temporario aguarda limpeza.");
 pkgi_rm("ux0:data/VitaStation/updates/cleanup-updater");
}
extern "C" void store_update_cleanup(){
 if(lock_id<0 || cover_thread_id<0 || !pkgi_file_exists("ux0:data/VitaStation/updates/cleanup-updater"))return;
 lock();if(update_busy || game_status.busy || progress.active || count){unlock();return;}cleanup_pending=true;update_busy=true;update_status.phase=6;unlock();sceKernelSignalSema(cover_wake_id,1);
}

extern "C" void store_update_fail(const char*message){if(lock_id<0)return;lock();update_status.phase=5;update_busy=false;snprintf(update_status.message,sizeof(update_status.message),"%s",message);unlock();}
