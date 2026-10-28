#ifndef VS_COVER_PACK_H
#define VS_COVER_PACK_H
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
bool cover_pack_available(void);
void cover_pack_reload(void);
bool cover_pack_find(const char*name,char*out,size_t capacity);
unsigned char*cover_pack_read(const char*path,size_t*size);
#ifdef __cplusplus
}
#endif
#endif
