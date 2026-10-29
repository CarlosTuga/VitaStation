#include "launch_handoff.h"
#include "../plugins/VitaStationBoot/shortcut.h"
#include "game_remove.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 unsigned char data[32];char caller[10],target[10],uri[32];assert(vs_launch_pack(data,"VTST00001","PCSE00001",100));assert(vs_launch_unpack(data,caller,target,200));assert(!strcmp(caller,"VTST00001")&&!strcmp(target,"PCSE00001"));vs_launch_uri(uri,target);assert(!strcmp(uri,"psgm:play?titleid=PCSE00001"));assert(!vs_launch_unpack(data,caller,target,6000000));assert(!vs_launch_pack(data,"OTHER0001","PCSE00001",0));assert(!vs_launch_pack(data,"VTST00001","VTST00001",0));
 VitaStationPSRoute route={0};assert(vs_ps_route(&route,1,0,1000000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,1100000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,1500000,1)==VS_PS_SINGLE);
 memset(&route,0,sizeof(route));assert(vs_ps_route(&route,1,1,2000000,1)==VS_PS_LIVEAREA);assert(vs_ps_route(&route,1,1,2100000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,2200000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,3000000,1)==VS_PS_NONE);assert(vs_ps_route(&route,1,0,4000000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,4100000,1)==VS_PS_NONE);assert(vs_ps_route(&route,0,0,4500000,1)==VS_PS_SINGLE);
 char path[256];bool dir;assert(game_remove_media_target("ux0:pspemu/ISO/test.iso",path,sizeof(path),&dir)&&!dir);assert(game_remove_media_target("uma0:pspemu/PSP/GAME/TEST/EBOOT.PBP",path,sizeof(path),&dir)&&dir&&!strcmp(path,"uma0:pspemu/PSP/GAME/TEST"));assert(!game_remove_media_target("ux0:pspemu/PSP/SAVEDATA/TEST/EBOOT.PBP",path,sizeof(path),&dir));assert(!game_remove_media_target("ux0:pspemu/ISO/../savedata.iso",path,sizeof(path),&dir));assert(!game_remove_id_allowed("VTST00001")&&!game_remove_id_allowed("NPXS10003")&&game_remove_id_allowed("PCSE00001"));
 puts("Shell handshake IDs/nonce/timeout/URI, PS alone unchanged, PS+L priority/no bounce, and bounded game deletion paths passed.");}
