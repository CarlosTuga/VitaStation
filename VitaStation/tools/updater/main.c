#include "launch_handoff.h"
#include "update_transaction.h"
/* Runs as VSTUP0001: VTST00001 is not the active application during promotion. */
#include "self_update.h"
#include <vita2d.h>
#include <psp2/sysmodule.h>
#include <psp2/promoterutil.h>
#include <psp2/appmgr.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static vita2d_pgf *font;
static void draw(const char*text,const char*detail){
 vita2d_start_drawing();vita2d_clear_screen();vita2d_draw_rectangle(0,0,960,544,RGBA8(13,23,41,255));
 vita2d_pgf_draw_text(font,45,108,RGBA8(245,248,255,255),1.5f,"Atualizar VitaStation");
 vita2d_pgf_draw_text(font,45,200,RGBA8(235,244,255,255),.95f,text);
 if(detail){char line[102];snprintf(line,sizeof(line),"%.100s",detail);vita2d_pgf_draw_text(font,45,258,RGBA8(182,206,232,255),.72f,line);}
 vita2d_pgf_draw_text(font,45,410,RGBA8(182,206,232,255),.8f,"Instalacao automatica. Mantem a consola ligada.");
 vita2d_end_drawing();vita2d_swap_buffers();sceDisplayWaitVblankStart();
}
static void result(const char*text){FILE*f=fopen("ux0:data/VitaStation/updates/install-result.txt","wb");if(f){fputs(text,f);fclose(f);}sceIoSync("ux0:",0);}
static int services(void){uint32_t args[]={0x400000,0xea60,0x40000,0,1,0};int code=0;SceSysmoduleOpt opt;memset(&opt,0,sizeof(opt));opt.result=&code;int n=sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,sizeof(args),args,&opt);if(n<0)return n;n=sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);return n<0?n:scePromoterUtilityInit();}
static int promote(const char*source,const char*ticket){
 char version[32],error[256];if(!self_update_verify(source,ticket,version,sizeof(version),error,sizeof(error))){result(error);return -1;}
 const char*shallow="ux0:data/vst_update";SceIoStat st;if(sceIoGetstat(shallow,&st)>=0){result("Existe uma recuperacao em ux0:data/vst_update. Preservada; instalacao bloqueada.");return -1;}
 int n=sceIoRename(source,shallow);if(n<0)return n;
 n=scePromoterUtilityPromotePkg(shallow,1);
 if(sceIoGetstat(shallow,&st)>=0){int moved=sceIoRename(shallow,source);if(n>=0 && moved<0)n=moved;}
 return n;
}
static char failure[256];
static bool verify_step(void*context,bool rollback){(void)context;char version[32];draw(rollback?"A verificar a copia de recuperacao...":"A verificar todos os recursos da atualizacao...",NULL);return self_update_verify(rollback?VS_ROLLBACK_ROOT:VS_UPDATE_ROOT,rollback?VS_ROLLBACK_TICKET:VS_UPDATE_TICKET,version,sizeof(version),failure,sizeof(failure));}
static int initialize_step(void*context){(void)context;return services();}
static int promote_step(void*context,bool rollback){(void)context;draw(rollback?"A tentar recuperar a versao anterior...":"A instalar o VitaStation e todos os recursos...",NULL);FILE*f=fopen("ux0:data/VitaStation/updates/installing","wb");if(f){fputs("VTST00001\n",f);fclose(f);}int n=promote(rollback?VS_ROLLBACK_ROOT:VS_UPDATE_ROOT,rollback?VS_ROLLBACK_TICKET:VS_UPDATE_TICKET);sceIoRemove("ux0:data/VitaStation/updates/installing");return n;}
static int restart_step(void*context){
 (void)context;result("VitaStation atualizado automaticamente.");FILE*f=fopen("ux0:data/VitaStation/library-dirty","wb");if(f){fputs("update\n",f);fclose(f);}
 f=fopen("ux0:data/VitaStation/updates/cleanup-updater","wb");if(f){fputs("VSTUP0001\n",f);fclose(f);}sceIoSync("ux0:",0);
 draw("Atualizacao concluida. A abrir o VitaStation...",NULL);return shell_handoff_request(VS_UPDATER_ID,"VTST00001")?0:-0x7001;
}
int main(void){
 vita2d_init();font=vita2d_load_default_pgf();if(!font)return 1;
 UpdateBackend backend={NULL,verify_step,initialize_step,promote_step,restart_step};int code=0;UpdateOutcome outcome=update_transaction(&backend,&code);
 if(outcome==UPDATE_OK)sceKernelExitProcess(0);
 if(outcome==UPDATE_RECOVERED)snprintf(failure,sizeof(failure),"Atualizacao falhou 0x%08X; versao anterior recuperada.",(unsigned)code);
 else if(outcome==UPDATE_RECOVERY_PENDING)snprintf(failure,sizeof(failure),"Atualizacao falhou 0x%08X; recuperacao preservada, mas pendente.",(unsigned)code);
 else if(outcome==UPDATE_RESTART_FAILED)snprintf(failure,sizeof(failure),"Instalado. Abertura recusada: 0x%08X. Abre VitaStation no LiveArea.",(unsigned)code);
 else if(outcome==UPDATE_INSTALLER_ERROR)snprintf(failure,sizeof(failure),"O sistema recusou o instalador: 0x%08X.",(unsigned)code);
 result(failure);
 while(1){draw("Consulta o resultado. O: voltar ao LiveArea.",failure);SceCtrlData pad;memset(&pad,0,sizeof(pad));if(sceCtrlPeekBufferPositive(0,&pad,1)>0 && pad.buttons&SCE_CTRL_CIRCLE)break;}
 sceKernelExitProcess(0);return 0;
}
