#include "psp_media.h"
#include "psp_runtime_hashes.h"
#include "installation_watch.h"
#include "settings.h"
#include <psp2/promoterutil.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <zlib.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <mutex>
#include <psp2/kernel/sysmem.h>
extern void vs_prepare_promoter();
extern void vs_prepare_bubble_head(const std::string&root,const char*id);
extern void vs_finish_bubble_staging(const std::string&root);
static std::mutex psp_mutex;
static const char*module_names[]={"adrenaline_kernel.skprx","adrenaline_user.suprx","adrenaline_vsh.suprx","adrbubblebooter.suprx","bootconv.suprx"};
static const char*module_root="ux0:app/PSPEMUCFW/sce_module/";
static std::vector<unsigned char>read(const std::string&path){FILE*f=fopen(path.c_str(),"rb");if(!f)throw std::runtime_error("Nao foi possivel ler "+path);if(fseek(f,0,SEEK_END)){fclose(f);throw std::runtime_error("Falha na leitura");}long n=ftell(f);if(n<0 || n>2*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);throw std::runtime_error("Ficheiro de arranque invalido");}std::vector<unsigned char>b((size_t)n);bool ok=fread(b.data(),1,b.size(),f)==b.size();fclose(f);if(!ok)throw std::runtime_error("Leitura incompleta");return b;}
static bool exists(const std::string&p){SceIoStat st;return sceIoGetstat(p.c_str(),&st)>=0;}
static unsigned crc(const std::vector<unsigned char>&b){return (unsigned)crc32(0,b.data(),b.size());}
static void write(const std::string&path,const std::vector<unsigned char>&b){FILE*f=fopen(path.c_str(),"wb");if(!f)throw std::runtime_error("Sem acesso para gravar "+path);bool ok=fwrite(b.data(),1,b.size(),f)==b.size();if(fclose(f))ok=false;if(!ok || read(path)!=b)throw std::runtime_error("A gravacao nao ficou completa");}
static long long boot_epoch(){SceRtcTick tick;if(sceRtcGetCurrentTick(&tick)<0)throw std::runtime_error("Nao foi possivel confirmar o reinicio");return (long long)((tick.tick-sceKernelGetSystemTimeWide())/1000000);}
static void prepare_runtime(){
 bool current=true;for(unsigned i=0;i<5;i++)if(!exists(std::string(module_root)+module_names[i]) || crc(read(std::string(module_root)+module_names[i]))!=runtime_crc[i])current=false;
 const char*reboot=VS_DATA_DIR "/psp-runtime-reboot.txt";
 if(current){
  FILE*f=fopen(reboot,"rb");if(f){long long epoch=0;bool valid=fscanf(f,"%lld",&epoch)==1;fclose(f);if(!valid || llabs(boot_epoch()-epoch)<5)throw std::runtime_error("Componente PSP preparado. Reinicia a consola antes de abrir o jogo.");remove(reboot);}return;
 }
 const unsigned official[]={0x20f7c5cdu,0x5487a9c3u,0xfa85cd74u};
 for(unsigned i=0;i<3;i++)if(!exists(std::string(module_root)+module_names[i]) || crc(read(std::string(module_root)+module_names[i]))!=official[i])throw std::runtime_error("Adrenaline nao reconhecido. A configuracao existente foi preservada.");
 for(unsigned i=3;i<5;i++)if(exists(std::string(module_root)+module_names[i]))throw std::runtime_error("Ja existe outro componente PSP. Usa a instalacao compativel do ABM.");
 settings_ensure_dir(VS_DATA_DIR "/psp-runtime-backup");
 for(unsigned i=0;i<5;i++){
  auto bytes=read(std::string("app0:assets/psp-runtime/")+module_names[i]);if(crc(bytes)!=runtime_crc[i])throw std::runtime_error("Componente PSP incluido corrompido");
  std::string target=std::string(module_root)+module_names[i];if(exists(target)){std::string backup=std::string(VS_DATA_DIR "/psp-runtime-backup/")+module_names[i];if(!exists(backup))write(backup,read(target));}
  write(target+".vst-new",bytes);
 }
 FILE*gate=fopen(reboot,"wb");if(!gate)throw std::runtime_error("Nao foi possivel guardar o pedido de reinicio.");bool gated=fprintf(gate,"%lld\n",boot_epoch())>0;if(fclose(gate))gated=false;if(!gated || sceIoSync("ux0:",0)<0){remove(reboot);throw std::runtime_error("Nao foi possivel confirmar o registo de reinicio.");}
 unsigned committed=0;std::string moved;
 try{for(unsigned i=0;i<5;i++){std::string target=std::string(module_root)+module_names[i];if(exists(target)){if(exists(target+".vst-previous"))throw std::runtime_error("Existe uma recuperacao PSP pendente");if(sceIoRename(target.c_str(),(target+".vst-previous").c_str())<0)throw std::runtime_error("Nao foi possivel preservar o modulo anterior");moved=target;}if(sceIoRename((target+".vst-new").c_str(),target.c_str())<0)throw std::runtime_error("Nao foi possivel instalar o componente PSP");moved.clear();committed++;}}
 catch(...){if(!moved.empty())sceIoRename((moved+".vst-previous").c_str(),moved.c_str());while(committed){std::string target=std::string(module_root)+module_names[--committed];sceIoRemove(target.c_str());if(exists(target+".vst-previous"))sceIoRename((target+".vst-previous").c_str(),target.c_str());}remove(reboot);throw;}
 sceIoSync("ux0:",0);
 throw std::runtime_error("Componente PSP instalado com copia de seguranca. Reinicia a consola.");
}
static unsigned le(const unsigned char*b){return b[0]|((unsigned)b[1]<<8)|((unsigned)b[2]<<16)|((unsigned)b[3]<<24);}
static void put32(unsigned char*b,unsigned v){for(unsigned i=0;i<4;i++)b[i]=(unsigned char)(v>>(i*8));}
static void field(std::vector<unsigned char>&sfo,const char*key,const char*value){unsigned k=le(sfo.data()+8),d=le(sfo.data()+12),n=le(sfo.data()+16);for(unsigned i=0;i<n;i++){unsigned char*e=sfo.data()+20+i*16;unsigned ko=e[0]|(e[1]<<8),cap=le(e+8),off=le(e+12);if(k+ko>=sfo.size() || d+off>sfo.size() || cap>sfo.size()-d-off)throw std::runtime_error("SFO de bolha invalido");if(!strcmp((char*)sfo.data()+k+ko,key)){size_t size=strlen(value);if(size>=cap)size=cap-1;while(size && ((unsigned char)value[size]&0xc0)==0x80)size--;memset(sfo.data()+d+off,0,cap);memcpy(sfo.data()+d+off,value,size);put32(e+4,size+1);return;}}throw std::runtime_error("Campo SFO ausente");}
extern "C" int psp_prepare_runtime(char*message,size_t capacity){std::lock_guard<std::mutex> guard(psp_mutex);try{prepare_runtime();snprintf(message,capacity,"Arranque PSP pronto.");return 0;}catch(const std::exception&e){snprintf(message,capacity,"%s",e.what());return exists(VS_DATA_DIR "/psp-runtime-reboot.txt")?1:-1;}}
extern "C" bool psp_create_bubble(const AppItem*item,char*message,size_t capacity){
 std::lock_guard<std::mutex> guard(psp_mutex);
 try{
  if(!item || !item->media_raw || !item->media_path[0] || !exists(item->media_path))throw std::runtime_error("O ficheiro do jogo deixou de estar disponivel.");
  const std::string installed=std::string("ux0:app/")+item->title_id;
  if(exists(installed+"/sce_sys/param.sfo")){
    if(!exists(installed+"/data/boot.bin"))throw std::runtime_error("O ID desta bolha ja pertence a outra aplicacao.");
    auto previous=read(installed+"/data/boot.bin");if(previous.size()!=320 || strcmp((char*)previous.data()+64,item->media_path))throw std::runtime_error("A bolha existente aponta para outro jogo.");return true;
  }
  prepare_runtime();
  std::string root="ux0:data/vst_psp";
  if(exists(root)){std::string recovery=std::string(VS_DATA_DIR "/psp-bubble-recovery-")+std::to_string(sceKernelGetSystemTimeWide());if(sceIoRename(root.c_str(),recovery.c_str())<0)throw std::runtime_error("Nao foi possivel preservar a preparacao PSP anterior.");}
  settings_ensure_dir(root.c_str());settings_ensure_dir((root+"/data").c_str());settings_ensure_dir((root+"/sce_sys").c_str());settings_ensure_dir((root+"/sce_sys/livearea").c_str());settings_ensure_dir((root+"/sce_sys/livearea/contents").c_str());
  auto boot=read("app0:assets/psp-runtime/bubble-boot.bin");if(boot.size()!=320 || strlen(item->media_path)>=256)throw std::runtime_error("Caminho PSP demasiado longo");memset(boot.data()+64,0,256);memcpy(boot.data()+64,item->media_path,strlen(item->media_path));write(root+"/data/boot.bin",boot);
  auto sfo=read("app0:assets/psp-runtime/bubble-param.sfo");field(sfo,"TITLE_ID",item->title_id);field(sfo,"TITLE",item->title);field(sfo,"STITLE",item->title);field(sfo,"APP_VER","01.00");write(root+"/sce_sys/param.sfo",sfo);
  write(root+"/eboot.bin",read("app0:assets/psp-runtime/bubble-eboot.bin"));write(root+"/sce_sys/icon0.png",read("app0:sce_sys/icon0.png"));
  const char*names[]={"template.xml","bg0.png","startup.png"};const char*assets[]={"bubble-template.xml","bubble-bg0.png","bubble-startup.png"};
  for(unsigned i=0;i<3;i++)write(root+"/sce_sys/livearea/contents/"+names[i],read(std::string("app0:assets/psp-runtime/")+assets[i]));
  vs_prepare_bubble_head(root,item->title_id);vs_prepare_promoter();int r=scePromoterUtilityPromotePkg(root.c_str(),1);if(r<0)throw std::runtime_error("A consola recusou criar a bolha PSP/PS1 ("+std::to_string(r)+")");
  vs_finish_bubble_staging(root);installation_mark_dirty();snprintf(message,capacity,"Bolha criada. A abrir o jogo.");return true;
 }catch(const std::exception&e){snprintf(message,capacity,"%s",e.what());return false;}
}
