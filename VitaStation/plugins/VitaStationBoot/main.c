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
#include <psp2/io/stat.h>
#include "../../include/version.h"
#include "shortcut.h"
#include "../../include/launch_handoff.h"
void *memset(void *p,int value,__SIZE_TYPE__ size){return sceClibMemset(p,value,size);}
static volatile int stopping=0;
static SceUID worker_id=-1;
static SceUID buttons_id=-1;
static SceUInt64 handoff_blocked_until;static int handoff_release_needed;
static void bridge_log(const char*event,const char*target,int code){
    SceIoStat stat;if(sceIoGetstat("ux0:data/VitaStation/bridge.log",&stat)>=0 && stat.st_size>65536)return;
    char line[128];int n=sceClibSnprintf(line,sizeof(line),"%llu %s %.9s 0x%08X\n",(unsigned long long)sceKernelGetSystemTimeWide(),event,target,(unsigned)code);if(n<=0 || n>=(int)sizeof(line))return;
    SceUID f=sceIoOpen("ux0:data/VitaStation/bridge.log",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_APPEND,0666);if(f>=0){sceIoWrite(f,line,n);sceIoClose(f);}
}
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
    bridge_log(event==VS_PS_SINGLE?"ps-single":"ps-double","VTST00001",result);
    char status[2]={(char)(event==VS_PS_SINGLE?'1':'2'),(char)(result<0?'E':'A')};
    SceUID fd=sceIoOpen("ux0:data/VitaStation/bridge.status",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(fd>=0){sceIoWrite(fd,status,2);sceIoClose(fd);}
}
static unsigned char launch_pending[32];static int launch_waiting=0;static SceUInt64 launch_accepted;
static uint64_t caller_gone_since;
static char probe_target[10];static SceUInt64 probe_at;
static int running(const char*target){SceUID direct_pid=-1;if(sceAppMgrGetIdByName(&direct_pid,target)>=0 && direct_pid>0)return 1;SceInt32 ids[16]={0};if(sceAppMgrGetRunningAppIdListForShell(ids,16)<0)return 1;for(unsigned i=0;i<16;i++)if(ids[i]>0){SceUID pid=sceAppMgrGetProcessIdByAppIdForShell(ids[i]);char name[16]={0};if(pid>0 && sceAppMgrGetNameById(pid,name)>=0 && vs_launch_same(name,target))return 1;}return 0;}
static void cancel_launch(void){launch_waiting=0;sceIoRemove("ux0:data/VitaStation/launch.request");sceIoRemove("ux0:data/VitaStation/launch.ack");}
static int request_still_pending(void){unsigned char bytes[33];SceUID f=sceIoOpen("ux0:data/VitaStation/launch.request",SCE_O_RDONLY,0);if(f<0)return 0;int n=sceIoRead(f,bytes,sizeof(bytes));sceIoClose(f);if(n!=32)return 0;for(unsigned i=0;i<32;i++)if(bytes[i]!=launch_pending[i])return 0;return 1;}
static void process_launch(SceUInt64 now){
 if(probe_at && now>=probe_at){bridge_log("probe-5s",probe_target,running(probe_target));probe_at=0;}
 if(marker("ux0:data/VitaStation/updates/installing")){if(running("VSTUP0001"))return;sceIoRemove("ux0:data/VitaStation/updates/installing");}
 char caller[10],target[10];
 if(launch_waiting){if(!request_still_pending() || !vs_launch_wait_unpack(launch_pending,caller,target,launch_accepted,now)){cancel_launch();return;}if(vs_launch_quiet_ready(&caller_gone_since,running(caller),now)){
  char uri[32];vs_launch_uri(uri,target);cancel_launch();handoff_blocked_until=now+3000000;handoff_release_needed=1;
  /* A positive Shell application ID did not start the target on the tested
     3.65 console. Use the explicit play URI, as for boot/PS routing, only
     after the caller has exited. Do not treat an allocated ID as execution. */
  int code=sceAppMgrLaunchAppByUri(VS_LAUNCH_PLAY_FLAGS,uri);bridge_log("launch-uri",target,code);
  for(unsigned i=0;i<10;i++)probe_target[i]=target[i];probe_at=now+5000000;
  SceUID status=sceIoOpen("ux0:data/VitaStation/launch-result.bin",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(status>=0){sceIoWrite(status,target,9);sceIoWrite(status,&code,sizeof(code));sceIoClose(status);}
 }return;}
 unsigned char bytes[33];SceUID f=sceIoOpen("ux0:data/VitaStation/launch.request",SCE_O_RDONLY,0);if(f<0)return;int n=sceIoRead(f,bytes,sizeof(bytes));sceIoClose(f);if(n!=32 || !vs_launch_unpack(bytes,caller,target,now)){cancel_launch();return;}
 f=sceIoOpen("ux0:data/VitaStation/launch.ack",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(f<0)return;int wrote=sceIoWrite(f,bytes,32);sceIoClose(f);if(wrote!=32)return;for(unsigned i=0;i<32;i++)launch_pending[i]=bytes[i];launch_accepted=now;caller_gone_since=0;launch_waiting=1;handoff_release_needed=1;bridge_log("ack",target,0);
}
static void request_livearea(SceUInt64 now){cancel_launch();SceUID f=sceIoOpen("ux0:data/VitaStation/livearea.request",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(f>=0){sceIoWrite(f,&now,sizeof(now));sceIoClose(f);}}
static int buttons_thread(SceSize argc,void*args){
    (void)argc;(void)args;
    VitaStationPSRoute routing={0};SceUInt64 latest=0,power_at=0,previous_poll=0;int power_sent=0,ps_enabled=0,power_enabled=0,routing_bypassed=0;unsigned poll=0;
    while(!stopping){
        SceUInt64 now=sceKernelGetSystemTimeWide();
        if(launch_waiting || handoff_release_needed || now<handoff_blocked_until)vs_ps_route_pause(&routing);
        static int version_written=0;if(!version_written){static const char label[]=VS_COMPONENTS_VERSION "\n";SceUID version=sceIoOpen("ux0:data/VitaStation/bridge-version.txt",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);if(version>=0){version_written=sceIoWrite(version,label,sizeof(label)-1)==sizeof(label)-1;sceIoClose(version);}}
        if(now-previous_poll>250000){power_at=0;power_sent=0;}previous_poll=now;
        if(poll++%20==0){ps_enabled=!routing_bypassed && marker("ux0:data/VitaStation/ps-routing.enabled");power_enabled=marker("ux0:data/VitaStation/power-menu.enabled");}
        SceCtrlData pads[16];int n=sceCtrlPeekBufferPositive2(0,pads,16);
        if(n>16)n=16;
        for(int i=0;i<n;i++){
            if(pads[i].timeStamp<=latest)continue;
            latest=pads[i].timeStamp;
            if(handoff_release_needed && !launch_waiting && now>=handoff_blocked_until && pads[i].timeStamp>=handoff_blocked_until && !(pads[i].buttons&SCE_CTRL_PSBUTTON))handoff_release_needed=0;
            if((pads[i].buttons&(SCE_CTRL_SELECT|SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER))==(SCE_CTRL_SELECT|SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER)){routing_bypassed=1;ps_enabled=0;}
            int event=vs_ps_route(&routing,(pads[i].buttons&SCE_CTRL_PSBUTTON)!=0,(pads[i].buttons&(SCE_CTRL_LTRIGGER|SCE_CTRL_L1))!=0,pads[i].timeStamp,ps_enabled && !launch_waiting && !handoff_release_needed && now>=handoff_blocked_until && !marker("ux0:data/VitaStation/updates/installing"));
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
        if(ps_enabled && !launch_waiting && !handoff_release_needed && now>=handoff_blocked_until && !marker("ux0:data/VitaStation/updates/installing"))route_ps(vs_ps_route(&routing,routing.ps.down,routing.left,now,ps_enabled));
        process_launch(now);
        sceKernelDelayThread(50000);
    }return sceKernelExitThread(0);
}
int _start(SceSize argc,const void*args) __attribute__((weak,alias("module_start")));
int module_start(SceSize argc,const void*args){
    (void)argc;(void)args;
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
