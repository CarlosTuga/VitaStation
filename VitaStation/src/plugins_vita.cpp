#include "plugins.h"
#include "store.h"
#include "../third_party/usagi/sha256.hpp"
#include "../third_party/usagi/utils.hpp"
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
extern "C" {
#include "log.h"
}
struct BundledPlugin{const char*file,*section,*sha256;};
#include "bundled_plugins.inc"
#include "bundled_psp.inc"
#include "boot_hash.h"
static bool exists(const std::string&p){SceIoStat st{};return sceIoGetstat(p.c_str(),&st)>=0;}
static std::vector<uint8_t> read_file(const std::string&path){
    FILE*f=fopen(path.c_str(),"rb");if(!f)throw std::runtime_error("Nao foi possivel ler um ficheiro de plugins.");
    if(fseek(f,0,SEEK_END)){fclose(f);throw std::runtime_error("Erro ao ler plugins.");}long size=ftell(f);
    if(size<0 || size>128*1024 || fseek(f,0,SEEK_SET)){fclose(f);throw std::runtime_error("Ficheiro de plugins demasiado grande.");}
    std::vector<uint8_t> data((size_t)size);bool ok=fread(data.data(),1,data.size(),f)==data.size();if(fclose(f))ok=false;if(!ok)throw std::runtime_error("Leitura incompleta de plugins.");return data;
}
static void write_new(const std::string&path,const std::vector<uint8_t>&data){
    if(exists(path))throw std::runtime_error("Um ficheiro existente foi preservado.");
    FILE*f=fopen(path.c_str(),"wb");if(!f)throw std::runtime_error("Nao foi possivel guardar plugins.");
    bool ok=fwrite(data.data(),1,data.size(),f)==data.size() && fflush(f)==0;if(fclose(f))ok=false;
    if(!ok || read_file(path)!=data)throw std::runtime_error("A verificacao do ficheiro de plugins falhou.");
}
static bool verify_hash(const std::vector<uint8_t>&data,const char*expected){uint8_t hash[32];sha256_ctx s;sha256_init(&s);sha256_update(&s,data.data(),data.size());sha256_finish(&s,hash);for(unsigned i=0;i<32;i++)if(hash[i]!=hexvalue(expected[i*2])*16+hexvalue(expected[i*2+1]))return false;return true;}
static void replace_config(const std::string&path,const std::vector<uint8_t>&before,const std::vector<uint8_t>&after){
    const auto suffix=".VitaStation-"+std::to_string(sceKernelGetProcessTimeWide());
    const auto backup=path+suffix+".bak",staged=path+suffix+".new",rotated=path+suffix+".previous";
    if(exists(path)){if(read_file(path)!=before)throw std::runtime_error("Configuracao alterada por outra aplicacao.");write_new(backup,before);}
    write_new(staged,after);bool had=exists(path);
    if(had && sceIoRename(path.c_str(),rotated.c_str())<0)throw std::runtime_error("Nao foi possivel guardar a configuracao anterior.");
    if(sceIoRename(staged.c_str(),path.c_str())<0){if(had)sceIoRename(rotated.c_str(),path.c_str());throw std::runtime_error("Nao foi possivel aplicar a configuracao.");}
    if(read_file(path)!=after){sceIoRemove(path.c_str());if(had)sceIoRename(rotated.c_str(),path.c_str());throw std::runtime_error("Falhou a verificacao da configuracao.");}
    if(had)sceIoRemove(rotated.c_str());
}
static bool prepare_psp(){
    const std::string root="ux0:pspemu/seplugins";bool changed=false;
    if(!exists("ux0:pspemu") && sceIoMkdir("ux0:pspemu",0777)<0)throw std::runtime_error("Nao foi possivel preparar PSP.");
    if(!exists(root) && sceIoMkdir(root.c_str(),0777)<0)throw std::runtime_error("Nao foi possivel preparar plugins PSP.");
    const bool original=exists(root+"/npdrm_free.prx");
    const std::string filename=original?"npdrm_free.prx":"npdrm_free_mod.prx";
    if(!original && !exists(root+"/"+filename)){
        auto data=read_file("app0:assets/plugins/npdrm_free_mod.prx");if(!verify_hash(data,psp_plugin_hash))throw std::runtime_error("O plugin PSP falhou a verificacao.");write_new(root+"/"+filename,data);changed=true;
    }
    for(const char*name:{"game.txt","vsh.txt"}){
        auto path=root+"/"+name;auto before=exists(path)?read_file(path):std::vector<uint8_t>{};std::string input(before.begin(),before.end());std::vector<char> output(128*1024);
        int result=plugins_psp_enable(input.c_str(),("ms0:/seplugins/"+filename).c_str(),output.data(),output.size());if(result<0)throw std::runtime_error("Conflito de plugins PSP. Configuracao preservada.");
        if(result){std::string text=output.data();replace_config(path,before,std::vector<uint8_t>(text.begin(),text.end()));changed=true;}
    }return changed;
}
extern "C" bool plugins_prepare(char*message,size_t size){
    try{
        SceKernelSystemSwVersion fw{};fw.size=sizeof(fw);int result=sceKernelGetSystemSwVersion(&fw);
        if(result<0)throw std::runtime_error("Nao foi possivel consultar a versao do sistema.");
        /* HENkaku can spoof the public firmware query. This bridge has no firmware
           offsets; do not block its installation on an exact spoofed value. */
        char firmware[96];snprintf(firmware,sizeof(firmware),"public firmware=0x%08X (may be spoofed)",(unsigned)fw.version);vs_log("plugins",firmware,0);
        const std::string root=exists("ux0:tai/config.txt")?"ux0:tai":"ur0:tai";
        const std::string config=root+"/config.txt";
        auto previous=read_file(config);std::string text(previous.begin(),previous.end());
        if(text.find('\0')!=std::string::npos)throw std::runtime_error("Configuracao taiHEN invalida.");
        if(text.find("nonpdrm_un")!=std::string::npos)throw std::runtime_error("Variante NoNpDrm detetada. Rever compatibilidade antes de alterar.");
        const std::string own=root+"/VitaStation";if(!exists(own) && sceIoMkdir(own.c_str(),0777)<0)throw std::runtime_error("Nao foi possivel criar a pasta de plugins.");
        bool changed=false;
        std::vector<BundledPlugin> all;
        if(text.find("AutoBoot.suprx")!=std::string::npos || text.find("VitaAutoLauncher.suprx")!=std::string::npos)throw std::runtime_error("Outro plugin de arranque detetado; configuracao preservada.");
        all.push_back({"VitaStationBoot.suprx","*main",VS_BOOT_SHA256});
        for(const auto&p:bundled_plugins)all.push_back(p);
        for(const auto&plugin:all){
            std::vector<PluginEntry> entries(128);size_t number=plugins_config_entries(text.c_str(),entries.data(),entries.size());bool disabled=false;
            for(size_t i=0;i<number;i++){const auto&entry=entries[i];const char*base=strrchr(entry.path,'/');base=base?base+1:entry.path;if(!entry.enabled && !strcmp(base,plugin.file))disabled=true;}
            if(disabled)continue; /* Preserve an explicit per-plugin choice. */
            auto data=read_file(std::string("app0:assets/plugins/")+plugin.file);
            if(!verify_hash(data,plugin.sha256))throw std::runtime_error("O plugin incluido falhou a verificacao SHA-256.");
            const std::string target=own+"/"+plugin.file;
            if(exists(target) && !verify_hash(read_file(target),plugin.sha256)){
                replace_config(target,read_file(target),data);changed=true;
            }
            std::vector<char> next(128*1024);int added=plugins_config_add(text.c_str(),plugin.section,target.c_str(),next.data(),next.size());
            if(added<0)throw std::runtime_error("Conflito numa entrada taiHEN; configuracao preservada.");
            if(added){
                if(exists(target)){if(!verify_hash(read_file(target),plugin.sha256))throw std::runtime_error("Existe outra versao na pasta VitaStation; preservada.");}
                else write_new(target,data);
                text=next.data();
                std::vector<uint8_t> updated(text.begin(),text.end());replace_config(config,previous,updated);previous=updated;changed=true;
            }
        }
        if(!changed){bool psp_changed=prepare_psp();snprintf(message,size,psp_changed?"Plugin PSP configurado. Reinicia o Adrenaline.":"Plugins necessarios ja configurados.");return true;}
        prepare_psp();snprintf(message,size,"Plugins configurados. Reinicia a consola para aplicar.");return true;
    }catch(const std::exception&e){vs_log("plugins",e.what(),-1);if(message && size)snprintf(message,size,"%s",e.what());return false;}
}
extern "C" size_t plugins_list(PluginEntry*entries,size_t capacity){
    try{
        auto bytes=read_file(exists("ux0:tai/config.txt")?"ux0:tai/config.txt":"ur0:tai/config.txt");std::string text(bytes.begin(),bytes.end());size_t count=plugins_config_entries(text.c_str(),entries,capacity);
        for(size_t i=0;i<count;i++)entries[i].present=exists(entries[i].path);
        return count;
    }catch(const std::exception&){return 0;}
}
extern "C" bool plugins_set_enabled(const PluginEntry*entry,bool enabled,char*message,size_t size){
    try{
        if(!entry || store_busy())throw std::runtime_error("Aguarda a conclusao das instalacoes antes de alterar plugins.");
        if(entry->protected_entry)throw std::runtime_error("Plugin essencial ao arranque ou armazenamento; protegido.");
        if(enabled && !exists(entry->path))throw std::runtime_error("O ficheiro deste plugin nao foi encontrado.");
        const std::string path=exists("ux0:tai/config.txt")?"ux0:tai/config.txt":"ur0:tai/config.txt";auto previous=read_file(path);std::string text(previous.begin(),previous.end());std::vector<char> output(128*1024);
        int changed=plugins_config_toggle(text.c_str(),entry,enabled,output.data(),output.size());if(changed<0)throw std::runtime_error("A configuracao mudou ou o plugin esta protegido.");
        if(changed){std::string next=output.data();replace_config(path,previous,std::vector<uint8_t>(next.begin(),next.end()));}
        if(message && size)snprintf(message,size,"Configuracao guardada. Reinicia para aplicar o plugin.");
        return true;
    }catch(const std::exception&e){if(message && size)snprintf(message,size,"%s",e.what());return false;}
}

/* Alive exposes explicit optional/required links. Install only the selected link. */
extern "C" bool plugins_install_catalog(const char*staged,const char*filename,const char*section,char*message,size_t size){
    try{
        bool known=false;for(const char*name:{"CapUnlocker.skprx","ttbr_remap.skprx","azaharnative.skprx","ioplus.skprx"})if(filename && !strcmp(name,filename))known=true;
        if(!known || !section || strcmp(section,"*KERNEL"))throw std::runtime_error("Este plugin do catalogo ainda nao foi validado.");
        auto data=read_file(staged);if(data.size()<4 || memcmp(data.data(),"SCE\0",4))throw std::runtime_error("O ficheiro descarregado nao e um modulo SCE valido.");
        const std::string root=exists("ux0:tai/config.txt")?"ux0:tai":"ur0:tai",config=root+"/config.txt",own=root+"/VitaStation";
        auto before=read_file(config);std::string text(before.begin(),before.end());std::vector<PluginEntry> entries(128);size_t count=plugins_config_entries(text.c_str(),entries.data(),entries.size());
        const auto target=own+"/"+filename;
        for(size_t i=0;i<count;i++){const auto&e=entries[i];const char*base=strrchr(e.path,'/');base=base?base+1:e.path;
            if(!strcmp(base,filename) && target!=e.path){
                if(e.enabled && exists(e.path)){snprintf(message,size,"Plugin existente preservado. Reinicia a consola se necessario.");return true;}
                throw std::runtime_error("Existe outra configuracao deste plugin; rever nas definicoes de plugins.");
            }
        }
        std::vector<char> output(128*1024);int changed=plugins_config_add(text.c_str(),section,target.c_str(),output.data(),output.size());if(changed<0)throw std::runtime_error("Conflito na configuracao taiHEN.");
        if(!exists(own) && sceIoMkdir(own.c_str(),0777)<0)throw std::runtime_error("Nao foi possivel criar a pasta de plugins.");
        if(exists(target)){if(read_file(target)!=data)replace_config(target,read_file(target),data);}else write_new(target,data);
        if(changed){std::string next=output.data();replace_config(config,before,std::vector<uint8_t>(next.begin(),next.end()));}
        snprintf(message,size,"Plugin configurado. Reinicia a consola para aplicar.");return true;
    }catch(const std::exception&e){if(message && size)snprintf(message,size,"%s",e.what());return false;}
}
