/* Exercises the production Shell worker with a simulated SDK/filesystem. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
typedef int SceUID;
typedef unsigned SceSize;
typedef int32_t SceInt32;
typedef uint64_t SceUInt64;
typedef struct {long long st_size;} SceIoStat;
typedef struct {uint64_t timeStamp;unsigned buttons;} SceCtrlData;
enum {SCE_O_RDONLY=1,SCE_O_WRONLY=2,SCE_O_CREAT=4,SCE_O_APPEND=8,SCE_O_TRUNC=16};
enum {SCE_CTRL_SELECT=1,SCE_CTRL_LTRIGGER=2,SCE_CTRL_RTRIGGER=4,SCE_CTRL_L1=8,SCE_CTRL_PSBUTTON=16,SCE_CTRL_POWER=32,SCE_KERNEL_START_SUCCESS=0,SCE_KERNEL_STOP_SUCCESS=0};
int sceIoGetstat(const char*,SceIoStat*);
int sceIoOpen(const char*,int,int);
int sceIoRead(int,void*,unsigned);
int sceIoWrite(int,const void*,unsigned);
int sceIoClose(int);
int sceIoRemove(const char*);
void *sceClibMemset(void*,int,size_t);
int sceClibSnprintf(char*,size_t,const char*,...);
SceUInt64 sceKernelGetSystemTimeWide(void);
int sceKernelExitThread(int);
int sceKernelDelayThread(unsigned);
int sceKernelCreateThread(const char*,int(*)(SceSize,void*),int,int,int,int,int);
int sceKernelStartThread(int,int,void*);
int sceKernelDeleteThread(int);
int sceKernelWaitThreadEnd(int,void*,void*);
int sceCtrlPeekBufferPositive(int,SceCtrlData*,int);
int sceCtrlPeekBufferPositive2(int,SceCtrlData*,int);
int sceAppMgrGetRunningAppIdListForShell(SceInt32*,int);
int sceAppMgrGetProcessIdByAppIdForShell(int);
int sceAppMgrGetNameById(int,char*);
int sceAppMgrGetIdByName(int*,const char*);
int sceAppMgrLaunchAppByUri(int,const char*);
/* Deliberately no LaunchAppByName2ForShell declaration: ID allocation is not
   the play-URI contract that this regression needs to exercise. */
#include "../plugins/VitaStationBoot/main.c"

typedef struct {const char *name;unsigned char data[2048];int n,pos,exists;} File;
static File files[]={
 {"ux0:data/VitaStation/launch.request",{0},0,0,0},
 {"ux0:data/VitaStation/launch.ack",{0},0,0,0},
 {"ux0:data/VitaStation/bridge.log",{0},0,0,0},
 {"ux0:data/VitaStation/launch-result.bin",{0},0,0,0}};
static int caller_alive=1,uri_calls,uri_result,target_alive;
static uint64_t test_now;
static char played_uri[64];
/* The handoff simulation never starts the infinite production worker loops. */
int sceKernelCreateThread(const char*n,int(*f)(SceSize,void*),int a,int b,int c,int d,int e){(void)n;(void)f;(void)a;(void)b;(void)c;(void)d;(void)e;abort();}
int sceKernelStartThread(int id,int n,void*p){(void)id;(void)n;(void)p;abort();}
int sceKernelDeleteThread(int id){(void)id;abort();}
int sceKernelWaitThreadEnd(int id,void*a,void*b){(void)id;(void)a;(void)b;abort();}
int sceKernelExitThread(int code){(void)code;abort();}
int sceKernelDelayThread(unsigned n){(void)n;abort();}
int sceCtrlPeekBufferPositive(int p,SceCtrlData*d,int n){(void)p;(void)d;(void)n;abort();}
int sceCtrlPeekBufferPositive2(int p,SceCtrlData*d,int n){(void)p;(void)d;(void)n;abort();}
int sceIoOpen(const char*p,int flags,int mode){(void)mode;for(unsigned i=0;i<4;i++)if(!strcmp(p,files[i].name)){File*f=&files[i];if(!f->exists && !(flags&SCE_O_CREAT))return -1;f->exists=1;if(flags&SCE_O_TRUNC)f->n=0;f->pos=(flags&SCE_O_APPEND)?f->n:0;return (int)i+1;}return -1;}
int sceIoRead(int id,void*p,unsigned n){File*f=&files[id-1];if(n>(unsigned)(f->n-f->pos))n=(unsigned)(f->n-f->pos);memcpy(p,f->data+f->pos,n);f->pos+=(int)n;return (int)n;}
int sceIoWrite(int id,const void*p,unsigned n){File*f=&files[id-1];assert(f->pos+(int)n<=2048);memcpy(f->data+f->pos,p,n);f->pos+=(int)n;if(f->pos>f->n)f->n=f->pos;return (int)n;}
int sceIoClose(int id){(void)id;return 0;}
int sceIoRemove(const char*p){for(unsigned i=0;i<4;i++)if(!strcmp(p,files[i].name)){files[i].exists=0;return 0;}return -1;}
int sceIoGetstat(const char*p,SceIoStat*s){for(unsigned i=0;i<4;i++)if(files[i].exists && !strcmp(p,files[i].name)){s->st_size=files[i].n;return 0;}return -1;}
void*sceClibMemset(void*p,int c,size_t n){volatile unsigned char*b=p;for(size_t i=0;i<n;i++)b[i]=(unsigned char)c;return p;}
int sceClibSnprintf(char*p,size_t n,const char*f,...){va_list a;va_start(a,f);int r=vsnprintf(p,n,f,a);va_end(a);return r;}
SceUInt64 sceKernelGetSystemTimeWide(void){return test_now;}
int sceAppMgrGetIdByName(int*pid,const char*name){*pid=(!strcmp(name,"VTST00001")?caller_alive:target_alive)?123:0;return 0;}
int sceAppMgrGetRunningAppIdListForShell(SceInt32*ids,int n){for(int i=0;i<n;i++)ids[i]=0;return 0;}
int sceAppMgrGetProcessIdByAppIdForShell(int id){(void)id;return -1;}
int sceAppMgrGetNameById(int id,char*p){(void)id;(void)p;return -1;}
int sceAppMgrLaunchAppByUri(int flags,const char*uri){assert(!caller_alive);assert(flags==0x20000);assert(!files[0].exists && !files[1].exists);uri_calls++;snprintf(played_uri,sizeof(played_uri),"%s",uri);return uri_result;}
static void request(uint64_t now){unsigned char packet[32];assert(vs_launch_pack(packet,"VTST00001","VITASHELL",now));File*f=&files[0];memcpy(f->data,packet,32);f->n=32;f->exists=1;test_now=now;process_launch(now);assert(launch_waiting && files[1].exists);assert(!memcmp(packet,files[1].data,32));}
int main(void){
 request(1000000);process_launch(1100000);assert(!uri_calls);
 caller_alive=0;process_launch(1200000);process_launch(1399999);assert(!uri_calls);
 test_now=1400000;process_launch(test_now);assert(uri_calls==1);assert(!strcmp(played_uri,"psgm:play?titleid=VITASHELL"));assert(!launch_waiting && handoff_release_needed);
 /* API acceptance does not assert that a process really started. */
 test_now=6400000;process_launch(test_now);assert(!target_alive && !probe_at && uri_calls==1);
 /* A failed URI call records the error, without trying a second launch. */
 caller_alive=1;request(7000000);caller_alive=0;process_launch(7100000);uri_result=-42;test_now=7300000;process_launch(test_now);assert(uri_calls==2);int recorded;memcpy(&recorded,files[3].data+9,sizeof(recorded));assert(recorded==-42);
 /* A cancelled client cannot cause a later launch. */
 caller_alive=1;request(8000000);files[0].exists=0;caller_alive=0;process_launch(8100000);assert(!launch_waiting && uri_calls==2);
 puts("Production Shell handoff: exact ACK, caller exit/stabilization, play URI/flags, result/probe and cancellation passed.");
 return 0;
}
