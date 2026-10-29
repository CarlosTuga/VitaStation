#ifndef VS_SELF_UPDATE_H
#define VS_SELF_UPDATE_H
#include <stdbool.h>
#include <stddef.h>
#define VS_UPDATE_ROOT "ux0:data/VitaStation/pkg-staging/self-update"
#define VS_ROLLBACK_ROOT "ux0:data/VitaStation/pkg-staging/update-rollback"
#define VS_UPDATE_TICKET "ux0:data/VitaStation/updates/install-ticket.json"
#define VS_ROLLBACK_TICKET "ux0:data/VitaStation/updates/rollback-ticket.json"
#define VS_UPDATER_ID "VSTUP0001"
#ifdef __cplusplus
extern "C" {
#endif
bool self_update_copy(const char *source,const char *destination,char *error,size_t capacity);
bool self_update_prepare(const char *root,const char *ticket,char *error,size_t capacity);
bool self_update_verify(const char *root,const char *ticket,char *version,size_t size,char *error,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
