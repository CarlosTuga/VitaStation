#ifndef VS_PSP_MEDIA_H
#define VS_PSP_MEDIA_H
#include "library.h"
#ifdef __cplusplus
extern "C" {
#endif
int psp_prepare_runtime(char*message,size_t capacity);
void psp_media_scan(Library*lib,const char*root);
void psp_identify_bubble(AppItem*item,const char*root);
void psp_livearea_scan(Library*lib);
bool psp_read_metadata(const char*path,AppItem*item);
bool psp_native_installed(const char*id);
bool psp_create_bubble(const AppItem*item,char*message,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
