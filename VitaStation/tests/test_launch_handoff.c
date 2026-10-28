#include <assert.h>
#include <string.h>
#include <stdbool.h>
#define __vita__ 1
static int named_result=0,uri_result=0,named_calls=0,uri_calls=0;static char launched[32];
int sceAppMgrLaunchAppByName(int flags,const char*id,const char*param){assert(flags==0x70000 && !param);named_calls++;strcpy(launched,id);return named_result;}
int sceAppMgrLaunchAppByUri(int flags,const char*uri){assert(flags==0x20000 && strstr(uri,"psgm:play?titleid=")==uri);uri_calls++;return uri_result;}
bool psp_native_installed(const char*id){return !strcmp(id,"NPEZ00175");}
void vs_log(const char*area,const char*message,int code){(void)area;(void)message;(void)code;}
#include "../src/launcher.c"
int main(void){assert(launcher_launch_app("NPEZ00175") && named_calls==1 && !uri_calls && !strcmp(launched,"NPEZ00175"));named_result=-1;assert(launcher_launch_app("NPEZ00175") && named_calls==2 && uri_calls==1);uri_result=-2;assert(!launcher_launch_app("NPEZ00175") && launcher_last_result().status==LAUNCH_REJECTED);assert(!launcher_launch_app("../unsafe") && named_calls==3);puts("Native PSP registration, direct EXIT_TO name launch and validated URI fallback passed (mocked AppMgr).");}
