#ifndef VS_GAME_REMOVE_H
#define VS_GAME_REMOVE_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
bool game_remove_id_allowed(const char *id);
bool game_remove_media_target(const char *path,char *target,size_t capacity,bool *directory);
#ifdef __cplusplus
}
#endif
#endif
