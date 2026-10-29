/* Original VitaStation shell bridge, MIT license. No firmware offsets or hooks.
 * Optional boot, PS gestures and a long-Power request to the application.
 * All routes require explicit markers. SELECT bypasses the optional boot.
 * Physical-button behavior must be validated on the target console. */
#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/clib.h>
#include "shortcut.h"
#include "../../include/launch_handoff.h"
void *memset(void *p,int value,__SIZE_TYPE__ size){return sceClibMemset(p,value,size);}
static volatile int stopping=0;
static SceUID worker_id=-1;
static SceUID buttons_id=-1;
static int marker(const char*path){
    static const char expected[]="VTST00001\n";char data[11]={0};
    SceUID fd=sceIoOpen(path,SCE_O_RDONLY,0);if(fd<0)return 0;
    int n=sceIoRead(fd,data,sizeof(data));sceIoClose(fd);if(n!=10)return 0;
    for(int i=0;i<10;i++)if(data[i]!=expected[i])return 0;
    return 1;
}
static int enabled(void){return marker("ux0:data/VitaStation/autoboot.enabled");}
static int bypass(void){SceCtrlData pad={0};return sceCtrlPeekBufferPositive(0,&pad,1)>0 && (pad.buttons&SCE_CTRL_SELECT);}
static int ready(void){
    SceInt32 ids[8]={0};if(sceAppMgrGetRunningAppIdListForShell(ids,8)<0)return 0;
    static const char target[]="NPXS10079";
    for(int i=0;i<8;i++)if(ids[i]>0){SceUID pid=sceAppMgrGetProcessIdByAppIdForShell(ids[i]);char name[16]={0};
        if(pid>0 && sceAppMgrGetNameById(pid,name)>=0){int same=1;for(int k=0;k<10;k++)if(name[k]!=target[k])same=0;if(same)return 1;}}
    return 0;
}
static int boot_thread(SceSize argc,void*args){
    (void)argc;(void)args;
    if(!enabled())return sceKernelExitThread(0);
    int shell_ready=0;
    for(int i=0;i<600 && !stopping;i++){
        if(!enabled() || bypass())return sceKernelExitThread(0);
        if(ready()){shell_ready=1;break;}sceKernelDelayThread(100000);
    }
    if(shell_ready){
        for(int i=0;i<10 && !stopping;i++){
            if(!enabled() || bypass())break;
            if(sceAppMgrLaunchAppByUri(VS_LAUNCH_PLAY_FLAGS,"psgm:play?titleid=VTST00001")>=0)break;
            sceKernelDelayThread(500000);
        }
    }
    return sceKernelExitThread(0);
}
static void route_ps(int event){
    if(event==VS_PS_NONE)return;
    const char*uri=event==VS_PS_SINGLE?"psgm:play?titleid=VTST00001":"psgm:open?titleid=VTST00001";
    int result=sceAppMgrLaunchAppByUri(event==VS_PS_SINGLE?VS_LAUNCH_PLAY_FLAGS:0,uri);
    char status[2]={(char)(event==VS_PS_SINGLE?'1':'2'),(char)(result<0?'E':'A')};
    SceUID fd=sceIoOpen("ux0:data/VitaStation/bridge.status",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(fd>=0){sceIoWrite(fd,status,2);sceIoClose(fd);}
}
static unsigned char launch_pending[32];static int launch_waiting=0;
static int running(const char*target){SceInt32 ids[16]={0};if(sceAppMgrGetRunningAppIdListForShell(ids,16)<0)return 1;for(unsigned i=0;i<16;i++)if(ids[i]>0){SceUID pid=sceAppMgrGetProcessIdByAppIdForShell(ids[i]);char name[16]={0};if(pid>0 && sceAppMgrGetNameById(pid,name)>=0 && vs_launch_same(name,target))return 1;}return 0;}
static void cancel_launch(void){launch_waiting=0;sceIoRemove("ux0:data/VitaStation/launch.request");sceIoRemove("ux0:data/VitaStation/launch.ack");}
static void process_launch(SceUInt64 now){
 if(marker("ux0:data/VitaStation/updates/installing"))return;
 char caller[10],target[10];
 if(launch_waiting){if(!vs_launch_unpack(launch_pending,caller,target,now)){cancel_launch();return;}if(!running(caller)){
  char uri[32];vs_launch_uri(uri,target);cancel_launch();int code=sceAppMgrLaunchAppByUri(VS_LAUNCH_PLAY_FLAGS,uri);
  SceUID status=sceIoOpen("ux0:data/VitaStation/launch-result.bin",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(status>=0){sceIoWrite(status,target,9);sceIoWrite(status,&code,sizeof(code));sceIoClose(status);}
 }return;}
 unsigned char bytes[33];SceUID f=sceIoOpen("ux0:data/VitaStation/launch.request",SCE_O_RDONLY,0);if(f<0)return;int n=sceIoRead(f,bytes,sizeof(bytes));sceIoClose(f);if(n!=32 || !vs_launch_unpack(bytes,caller,target,now)){cancel_launch();return;}
 f=sceIoOpen("ux0:data/VitaStation/launch.ack",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(f<0)return;int wrote=sceIoWrite(f,bytes,32);sceIoClose(f);if(wrote!=32)return;for(unsigned i=0;i<32;i++)launch_pending[i]=bytes[i];launch_waiting=1;
}
static void request_livearea(SceUInt64 now){cancel_launch();SceUID f=sceIoOpen("ux0:data/VitaStation/livearea.request",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(f>=0){sceIoWrite(f,&now,sizeof(now));sceIoClose(f);}}
static int buttons_thread(SceSize argc,void*args){
    (void)argc;(void)args;
    VitaStationPSRoute routing={0};SceUInt64 latest=0,power_at=0,previous_poll=0;int power_sent=0,ps_enabled=0,power_enabled=0,routing_bypassed=0;unsigned poll=0;
    while(!stopping){
        SceUInt64 now=sceKernelGetSystemTimeWide();
        if(now-previous_poll>250000){power_at=0;power_sent=0;}previous_poll=now;
        if(poll++%20==0){ps_enabled=!routing_bypassed && marker("ux0:data/VitaStation/ps-routing.enabled");power_enabled=marker("ux0:data/VitaStation/power-menu.enabled");}
        SceCtrlData pads[16];int n=sceCtrlPeekBufferPositive2(0,pads,16);
        if(n>16)n=16;
        for(int i=0;i<n;i++){
            if(pads[i].timeStamp<=latest)continue;
            latest=pads[i].timeStamp;
            if((pads[i].buttons&(SCE_CTRL_SELECT|SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER))==(SCE_CTRL_SELECT|SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER)){routing_bypassed=1;ps_enabled=0;}
            int event=vs_ps_route(&routing,(pads[i].buttons&SCE_CTRL_PSBUTTON)!=0,(pads[i].buttons&(SCE_CTRL_LTRIGGER|SCE_CTRL_L1))!=0,pads[i].timeStamp,ps_enabled && !marker("ux0:data/VitaStation/updates/installing"));
            if(event==VS_PS_LIVEAREA)request_livearea(now);else route_ps(event);
            if(power_enabled && (pads[i].buttons&SCE_CTRL_POWER)){
                if(!power_at)power_at=now;
                if(!power_sent && now-power_at>=1500000){
                    SceUID fd=sceIoOpen("ux0:data/VitaStation/power.request",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);
                    if(fd>=0){sceIoWrite(fd,"1",1);sceIoClose(fd);sceAppMgrLaunchAppByUri(VS_LAUNCH_PLAY_FLAGS,"psgm:play?titleid=VTST00001");}
                    power_sent=1;
                }
            }else{power_at=0;power_sent=0;}
        }
        if(ps_enabled && !marker("ux0:data/VitaStation/updates/installing"))route_ps(vs_ps_route(&routing,routing.ps.down,routing.left,now,ps_enabled));
        process_launch(now);
        sceKernelDelayThread(50000);
    }return sceKernelExitThread(0);
}
int _start(SceSize argc,const void*args) __attribute__((weak,alias("module_start")));
int module_start(SceSize argc,const void*args){
    (void)argc;(void)args;
    SceUID version=sceIoOpen("ux0:data/VitaStation/bridge-version.txt",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(version>=0){sceIoWrite(version,"1.6.6\n",6);sceIoClose(version);}
    worker_id=sceKernelCreateThread("VitaStationBoot",boot_thread,0x80,64*1024,0,0,0);
    if(worker_id>=0 && sceKernelStartThread(worker_id,0,0)<0){sceKernelDeleteThread(worker_id);worker_id=-1;}
    buttons_id=sceKernelCreateThread("VitaStationButtons",buttons_thread,0x10000100,64*1024,0,0,0);
    if(buttons_id>=0 && sceKernelStartThread(buttons_id,0,0)<0){sceKernelDeleteThread(buttons_id);buttons_id=-1;}
    return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc,const void*args){
    (void)argc;(void)args;stopping=1;
    if(worker_id>=0){sceKernelWaitThreadEnd(worker_id,0,0);sceKernelDeleteThread(worker_id);worker_id=-1;}
    if(buttons_id>=0){sceKernelWaitThreadEnd(buttons_id,0,0);sceKernelDeleteThread(buttons_id);buttons_id=-1;}
    return SCE_KERNEL_STOP_SUCCESS;
}
