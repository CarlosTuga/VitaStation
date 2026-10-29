#include "launch_handoff.h"
#include <string.h>
#if defined(__vita__)
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#endif
int shell_handoff_request(const char*caller,const char*target){
#if defined(__vita__)
 unsigned char request[32],ack[33];if(!vs_launch_pack(request,caller,target,sceKernelGetSystemTimeWide()))return 0;
 const char*path="ux0:data/VitaStation/launch.request",*temp="ux0:data/VitaStation/launch.request.new",*reply="ux0:data/VitaStation/launch.ack";
 sceIoRemove(reply);SceUID f=sceIoOpen(temp,SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(f<0)return 0;int ok=sceIoWrite(f,request,32)==32;if(sceIoClose(f)<0)ok=0;if(!ok)return 0;sceIoRemove(path);if(sceIoRename(temp,path)<0)return 0;
 for(unsigned i=0;i<60;i++){f=sceIoOpen(reply,SCE_O_RDONLY,0);if(f>=0){int n=sceIoRead(f,ack,sizeof(ack));sceIoClose(f);if(n==32 && !memcmp(ack,request,32))return 1;}sceKernelDelayThread(10000);}
 sceIoRemove(path);sceIoRemove(reply);return 0;
#else
 (void)caller;(void)target;return 0;
#endif
}
