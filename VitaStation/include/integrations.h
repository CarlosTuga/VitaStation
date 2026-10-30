#ifndef VS_INTEGRATIONS_H
#define VS_INTEGRATIONS_H
#include "library.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {char name[96],file[96],section[32],version[24];unsigned crc;bool automatic;} OnlinePlugin;
typedef struct {bool busy;unsigned count;char message[256];} IntegrationStatus;
size_t system_apps_scan(AppItem *items,size_t capacity);
void system_apps_populate(Library *library,const char *cache_path);
void store_autoplugin_request(int index);
void store_integration_status(IntegrationStatus *status);
bool store_autoplugin_item(int index,OnlinePlugin *item);
void store_apollo_request(void);
bool store_apollo_ready(void);
#ifdef __cplusplus
}
#endif
#endif
