#ifndef VS_PLUGINS_H
#define VS_PLUGINS_H
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Pure taiHEN configuration transform: preserve unrelated sections and avoid duplicate modules.
 * Returns 1 when added, 0 when already present, -1 on conflict or invalid input. */
int plugins_config_add(const char *input,const char *section,const char *path,char *output,size_t capacity);
int plugins_psp_enable(const char *input,const char *path,char *output,size_t capacity);
typedef struct {char path[256],section[32];bool enabled,present,protected_entry;} PluginEntry;
size_t plugins_config_entries(const char *input,PluginEntry *entries,size_t capacity);
int plugins_config_toggle(const char *input,const PluginEntry *entry,bool enabled,char *output,size_t capacity);
size_t plugins_list(PluginEntry *entries,size_t capacity);
bool plugins_set_enabled(const PluginEntry *entry,bool enabled,char *message,size_t size);
/* Install only bundled dependencies. Never restores the personal all-plugins backup. */
bool plugins_prepare(char *message,size_t size);
#ifdef __cplusplus
}
#endif
#endif
