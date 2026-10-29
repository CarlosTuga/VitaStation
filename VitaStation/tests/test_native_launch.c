/* Exercise the real Vita adapter: queue, then direct URI at the exit boundary. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/launcher.c"
static int calls,removals,reply;
static char launched[64];
bool psp_native_installed(const char*id){return !strcmp(id,"VITASHELL");}
void vs_log(const char*a,const char*b,int c){(void)a;(void)b;(void)c;}
int sceIoRemove(const char*p){assert(!strcmp(p,"ux0:data/VitaStation/launch.request") || !strcmp(p,"ux0:data/VitaStation/launch.ack"));removals++;return 0;}
int sceAppMgrLaunchAppByUri(int flags,const char*uri){assert(flags==0xFFFFF);assert(removals==2*(calls+1));calls++;snprintf(launched,sizeof(launched),"%s",uri);return reply;}
int main(void){
 assert(launcher_launch_app("VITASHELL"));assert(launcher_exit_required() && !calls && !removals);
 assert(launcher_commit_exit());assert(calls==1 && launcher_exit_required());assert(!strcmp(launched,"psgm:play?titleid=VITASHELL"));
 assert(launcher_commit_exit() && calls==1); /* No duplicate URI. */
 assert(launcher_launch_app("VITASHELL") && calls==1);reply=-42;
 assert(!launcher_commit_exit());assert(!launcher_exit_required() && calls==2);assert(launcher_last_result().status==LAUNCH_REJECTED && launcher_last_result().platform_code==-42);
 assert(!launcher_launch_app("../bad") && !launcher_exit_required() && calls==2);
 puts("Native production adapter: no Shell ACK dependency, deferred direct launch, stale handoff cancellation, one URI, error keeps caller alive passed.");
}
