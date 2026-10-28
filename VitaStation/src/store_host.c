#include "updater.h"
#include "store.h"
#include "system_info.h"
#include <string.h>
#include <stdio.h>
#if !defined(__vita__)
void store_update_request(void){}
void store_update_status(UpdateStatus*s){if(s){memset(s,0,sizeof(*s));snprintf(s->message,sizeof(s->message),"Atualizacoes exigem uma PS Vita.");}}
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
