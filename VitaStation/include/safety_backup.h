#ifndef VS_SAFETY_BACKUP_H
#define VS_SAFETY_BACKUP_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
bool safety_backup_exists(void);
size_t safety_backup_plan(char lines[][128],size_t capacity);
bool safety_backup_ensure(char *message,size_t capacity);
bool safety_backup_track_plugin(const char *path,char *message,size_t capacity);
bool safety_backup_remove_additions(char *message,size_t capacity);
bool safety_backup_restore(char *message,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
