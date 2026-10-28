#ifndef VS_SYSTEM_INFO_H
#define VS_SYSTEM_INFO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {SYS_INFO_NETWORK=100,SYS_INFO_CONSOLE=101,SYS_INFO_BATTERY=102,SYS_INFO_STORAGE=103,SYS_INFO_CLOCK=104,SYS_INFO_LOG=105,SYS_INFO_INPUT=106,SYS_INFO_LICENSES=107,SYS_INFO_PERFORMANCE=108,SYS_INFO_DOWNLOADS=109,SYS_INFO_LANGUAGE=110};
size_t system_info_read(int page,char lines[6][128]);
size_t system_network_describe(char lines[6][128]);
#ifdef __cplusplus
}
#endif
#endif
