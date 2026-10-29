#include "safety_backup.h"
#include "bootstrap.h"
#include "library_cache.h"
#include "settings.h"
#include "plugins.h"
#include "psp_media.h"
#include "system_audit.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#if defined(__vita__)
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
static SceUID thread=-1,mutex=-1;
static Library pending;
static bool library_ready=false,plugins_ready=false,prepare_success=false;
static int result=-1;
static char message[256];
static int load(SceSize size,void*arg){
 (void)size;(void)arg;char backup[256];safety_backup_ensure(backup,sizeof(backup));uint64_t start=sceKernelGetProcessTimeWide();int scanned=library_scan(&pending,"ux0:app/");
 if(scanned>=0){library_load_userdata(&pending,VS_USERDATA_PATH);library_cache_save(&pending,VS_DATA_DIR "/library-cache.bin");}
 char timing[96];snprintf(timing,sizeof(timing),"library_ready_ms=%llu apps=%d",(unsigned long long)((sceKernelGetProcessTimeWide()-start)/1000),scanned);vs_log("startup",timing,scanned);
 sceKernelLockMutex(mutex,1,NULL);result=scanned;library_ready=true;sceKernelUnlockMutex(mutex,1);
 start=sceKernelGetProcessTimeWide();char text[256];prepare_success=plugins_prepare(text,sizeof(text));
 system_settings_audit();
 char psp[256];int psp_result=psp_prepare_runtime(psp,sizeof(psp));vs_log("psp-runtime",psp,psp_result);if(psp_result==1)snprintf(text,sizeof(text),"%s",psp);
 snprintf(timing,sizeof(timing),"plugin_prepare_ms=%llu",(unsigned long long)((sceKernelGetProcessTimeWide()-start)/1000));vs_log("startup",timing,0);
 sceKernelLockMutex(mutex,1,NULL);snprintf(message,sizeof(message),"%s",text);plugins_ready=true;sceKernelUnlockMutex(mutex,1);
 return sceKernelExitThread(0);
}
bool bootstrap_start(const Library*initial){
 if(!initial || thread>=0)return false;
 pending=*initial;library_ready=plugins_ready=prepare_success=false;mutex=sceKernelCreateMutex("vs-startup",0,0,NULL);if(mutex<0)return false;
 thread=sceKernelCreateThread("vs-library-loader",load,0x10000110,256*1024,0,0,NULL);
 if(thread>=0 && sceKernelStartThread(thread,0,NULL)>=0)return true;
 if(thread>=0)sceKernelDeleteThread(thread);
 sceKernelDeleteMutex(mutex);thread=mutex=-1;return false;
}
unsigned bootstrap_poll(Library*lib,int*out,char*text,size_t size){
 if(mutex<0)return 0;
 unsigned flags=0;sceKernelLockMutex(mutex,1,NULL);
 if(library_ready){library_ready=false;if(out)*out=result;if(result>=0 && lib){
  for(int i=0;i<pending.count;i++)for(int j=0;j<lib->count;j++)if(!strcmp(pending.items[i].title_id,lib->items[j].title_id)){
   pending.items[i].is_favorite=lib->items[j].is_favorite;pending.items[i].last_played=lib->items[j].last_played;pending.items[i].launch_count=lib->items[j].launch_count;
   if(lib->items[j].classification_override){pending.items[i].classification_override=true;pending.items[i].type=lib->items[j].type;pending.items[i].section=lib->items[j].section;}break;
  }
  library_update_indices(&pending);*lib=pending;
 }flags|=1;}
 if(plugins_ready){plugins_ready=false;if(text && size)snprintf(text,size,"%s",message);flags|=2;if(prepare_success)flags|=8;}
 sceKernelUnlockMutex(mutex,1);return flags;
}
bool bootstrap_busy(void){return thread>=0;}
void bootstrap_shutdown(void){if(thread>=0){sceKernelWaitThreadEnd(thread,NULL,NULL);sceKernelDeleteThread(thread);}if(mutex>=0)sceKernelDeleteMutex(mutex);thread=mutex=-1;}
#else
bool bootstrap_start(const Library*initial){(void)initial;return false;}
unsigned bootstrap_poll(Library*lib,int*result,char*text,size_t size){(void)lib;(void)result;(void)text;(void)size;return 0;}
bool bootstrap_busy(void){return false;}
void bootstrap_shutdown(void){}
#endif
