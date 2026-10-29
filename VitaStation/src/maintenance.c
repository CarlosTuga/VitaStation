#include "bootstrap.h"
#include "maintenance.h"
#include "autoboot.h"
#include "settings.h"
#include "store.h"
#include <stdio.h>
#if defined(__vita__)
#include <psp2/appmgr.h>
#endif
bool maintenance_uninstall(bool restore,char*error,size_t cap){
 if(store_busy() || bootstrap_busy()){snprintf(error,cap,"Aguarda as descargas e instalacoes locais.");return false;}
 if(!autoboot_set(false) || !autoboot_marker_write(VS_DATA_DIR "/ps-routing.enabled",false) || !autoboot_marker_write(VS_DATA_DIR "/power-menu.enabled",false)){
  snprintf(error,cap,"Nao foi possivel desativar o arranque. Remocao cancelada.");return false;
 }
#if defined(__vita__)
 store_shutdown();char*args[]={"maintenance",restore?"restore":"simple",NULL};int result=sceAppMgrLoadExec("app0:maintenance.self",args,NULL);
 snprintf(error,cap,"Nao foi possivel abrir o desinstalador: 0x%08X. Reabre o launcher.",(unsigned)result);return false;
#else
 (void)restore;snprintf(error,cap,"A desinstalacao exige uma PS Vita.");return false;
#endif
}
