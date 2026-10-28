#ifndef VS_COVER_PACK_H
#define VS_COVER_PACK_H
#include <stddef.h>
#include <stdbool.h>
bool cover_pack_find(const char*name,char*out,size_t capacity);
unsigned char*cover_pack_read(const char*path,size_t*size);
#endif
