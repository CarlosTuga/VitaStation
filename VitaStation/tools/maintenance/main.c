#include "safety_backup.h"
/* VitaStation maintenance executable. Runs after replacing the launcher process. */
#include <vita2d.h>
#include <psp2/sysmodule.h>
#include <psp2/promoterutil.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define NOT_FOUND ((int)0x80010002)
static vita2d_pgf*font;
static void draw(const char*message,int code){
 vita2d_start_drawing();vita2d_clear_screen();vita2d_draw_rectangle(0,0,960,544,RGBA8(13,23,41,255));
 vita2d_pgf_draw_text(font,50,110,RGBA8(245,248,255,255),1.4f,"Desinstalar VitaStation");
 vita2d_pgf_draw_text(font,50,220,RGBA8(225,238,255,255),.9f,message);
 if(code){char text[100];snprintf(text,sizeof(text),"Erro: 0x%08X. O: sair; podes repetir pelo launcher.",(unsigned)code);vita2d_pgf_draw_text(font,50,300,RGBA8(255,196,170,255),.7f,text);}
 vita2d_pgf_draw_text(font,50,390,RGBA8(180,200,224,255),.8f,"O arranque original fica reposto. Nao desligues durante a remocao.");
 vita2d_end_drawing();vita2d_swap_buffers();sceDisplayWaitVblankStart();
}
static int remove_if_present(const char*path){SceIoStat st;int n=sceIoGetstat(path,&st);if(n==NOT_FOUND)return 0;if(n<0)return n;return sceIoRemove(path);}
static int disable_markers(void){const char*paths[]={"ux0:data/VitaStation/autoboot.enabled","ux0:data/VitaStation/ps-routing.enabled","ux0:data/VitaStation/power-menu.enabled","ux0:data/VitaStation/power.request"};for(unsigned i=0;i<4;i++){int n=remove_if_present(paths[i]);if(n<0)return n;}return sceIoSync("ux0:",0);}
static int remove_tree(const char*path,unsigned depth){
 const char*root="ux0:data/VitaStation/covers";size_t len=strlen(root);
 if(strncmp(path,root,len) || (path[len] && path[len]!='/') || strstr(path,"..") || depth>20)return -1;
 SceIoStat st;int result=sceIoGetstat(path,&st);if(result==NOT_FOUND)return 0;if(result<0)return result;
 if(!SCE_S_ISDIR(st.st_mode))return sceIoRemove(path);
 SceUID dir=sceIoDopen(path);if(dir<0)return dir;SceIoDirent entry;int n=0;
 while(1){memset(&entry,0,sizeof(entry));n=sceIoDread(dir,&entry);if(n<=0)break;if(!strcmp(entry.d_name,".") || !strcmp(entry.d_name,".."))continue;
  if(strchr(entry.d_name,'/') || strchr(entry.d_name,'\\')){n=-1;break;}char child[1024];if(snprintf(child,sizeof(child),"%s/%s",path,entry.d_name)>=(int)sizeof(child)){n=-1;break;}
  n=remove_tree(child,depth+1);if(n<0)break;sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
 }
 sceIoDclose(dir);if(n<0)return n;return sceIoRmdir(path);
}
#include "config_cleanup.h"
static int delete_installed(const char*id){char path[100];snprintf(path,sizeof(path),"ux0:app/%s/sce_sys/param.sfo",id);SceIoStat st;int n=sceIoGetstat(path,&st);if(n==NOT_FOUND)return 0;if(n<0)return n;return scePromoterUtilityDeletePkg(id);}
int main(int argc,char**argv){
 bool restore=argc>1 && !strcmp(argv[1],"restore");
 vita2d_init();font=vita2d_load_default_pgf();if(!font)return 1;draw("A repor o arranque original...",0);int result=disable_markers();bool reset=result>=0;char backup_message[256]={0};
 if(result>=0 && restore){draw("A verificar e repor o backup de plugins...",0);if(!safety_backup_restore(backup_message,sizeof(backup_message)) || !safety_backup_remove_additions(backup_message,sizeof(backup_message)))result=-1;}
 if(result>=0)result=disable_config("ur0:tai/config.txt");
 if(result>=0)result=disable_config("ux0:tai/config.txt");
 if(result>=0){uint32_t args[]={0x400000,0xea60,0x40000,0,1,0};int code=0;SceSysmoduleOpt opt;memset(&opt,0,sizeof(opt));opt.result=&code;result=sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,sizeof(args),args,&opt);}
 if(result>=0)result=sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
 if(result>=0)result=scePromoterUtilityInit();
 if(result>=0)result=delete_installed("VSTUP0001");
 if(result>=0){draw("A remover o instalador de caratulas...",0);result=delete_installed("VSCV00001");}
 if(result>=0){remove_if_present("ux0:data/VitaStation/updates/Caratulas-de-loja-1.04.vpk.part");remove_if_present("ux0:data/VitaStation/updates/Caratulas-de-loja-1.04.vpk");draw("A remover todas as caratulas...",0);result=remove_tree("ux0:data/VitaStation/covers",0);}
 if(result>=0){remove_if_present("ur0:tai/VitaStation/VitaStationBoot.suprx");remove_if_present("ux0:tai/VitaStation/VitaStationBoot.suprx");sceIoSync("ux0:",0);sceIoSync("ur0:",0);draw("A remover o VitaStation...",0);result=delete_installed("VTST00001");}
 if(result>=0){draw("VitaStation removido. Reinicia para concluir a reposicao.",0);sceKernelDelayThread(2500000);}
 else{while(1){draw(reset?(backup_message[0]?backup_message:"Remocao incompleta; o arranque automatico fica desligado."):"Falha a repor o arranque; remocao cancelada.",result);SceCtrlData pad;memset(&pad,0,sizeof(pad));if(sceCtrlPeekBufferPositive(0,&pad,1)>0 && pad.buttons&SCE_CTRL_CIRCLE)break;}}
 sceKernelExitProcess(result<0?1:0);return 0;
}
