#ifndef VS_GAME_SUPPORT_H
#define VS_GAME_SUPPORT_H
#include "library.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {bool busy;int mod_count;char message[256],mods[5][128];} GameSupportStatus;
void store_game_request(const AppItem *app,int action,int selection);
void store_game_status(GameSupportStatus *status);
void store_request_background(const char *id);
#ifdef __cplusplus
}
#endif
#endif
