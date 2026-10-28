#ifndef VS_BGDL_PAYLOAD_H
#define VS_BGDL_PAYLOAD_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {int32_t type[2];char reserved[0x68];char url[0x800];char icon_path[0x100];char title[0x33a];char license_path[0x100];char tail[0x16];} BgdlPayload;
bool bgdl_payload_build(BgdlPayload*payload,int type,const char*title,const char*url,const char*license,const char*icon);
#ifdef __cplusplus
}
#endif
#endif
