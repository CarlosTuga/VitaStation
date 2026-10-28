#ifndef VS_INSTALLATION_WATCH_H
#define VS_INSTALLATION_WATCH_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
uint64_t installation_signature(void);
bool installation_cache_changed(const char*path);
bool installation_cache_commit(const char*path);
void installation_mark_dirty(void);
bool installation_dirty(void);
void installation_clear_dirty(void);
#ifdef __cplusplus
}
#endif
#endif
