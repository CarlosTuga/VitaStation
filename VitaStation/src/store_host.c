#include "integrations.h"
#include "game_support.h"
#include "updater.h"
#include "store.h"
#include "system_info.h"
#include <string.h>
#include <stdio.h>
#if !defined(__vita__)
bool store_update_install(char*error,size_t n){if(error && n)snprintf(error,n,"Atualizacao exige uma PS Vita.");return false;}
void store_update_fail(const char*m){(void)m;}
void store_update_cleanup(void){}
void store_update_request(void){}
void store_update_status(UpdateStatus*s){if(s){memset(s,0,sizeof(*s));snprintf(s->message,sizeof(s->message),"Atualizacoes exigem uma PS Vita.");}}
void store_autoplugin_request(int i){(void)i;}
void store_integration_status(IntegrationStatus*s){memset(s,0,sizeof(*s));snprintf(s->message,sizeof(s->message),"Integracao de rede exige uma PS Vita.");}
bool store_autoplugin_item(int i,OnlinePlugin*p){(void)i;(void)p;return false;}
void store_apollo_request(void){}
bool store_apollo_ready(void){return false;}
void store_game_request(const AppItem*a,int action,int selection){(void)a;(void)action;(void)selection;}
void store_game_status(GameSupportStatus*s){memset(s,0,sizeof(*s));snprintf(s->message,sizeof(s->message),"Funcao de rede disponivel na PS Vita.");}
void store_request_background(const char*id){(void)id;}
bool store_start(void){return false;}
bool store_enqueue(const CatalogPackage*p,char*error,size_t n){(void)p;if(error && n)snprintf(error,n,"A instalacao exige uma PS Vita.");return false;}
void store_progress(StoreProgress*p){if(p)memset(p,0,sizeof(*p));}
void store_cancel(void){}
bool store_busy(void){return false;}
void store_request_cover(CatalogType type,const char*id){(void)type;(void)id;}
void store_retry_covers(void){}
void store_request_image(CatalogType type,const char*key,const char*url){(void)type;(void)key;(void)url;}
void store_refresh_homebrew(bool force){(void)force;}
void store_shutdown(void){}
size_t system_network_describe(char lines[6][128]){(void)lines;return 0;}
#endif
