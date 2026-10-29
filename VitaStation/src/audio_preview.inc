/* Selection preview decoder: SDK APIs only, no firmware patches/plugins. */
#include <psp2/atrac.h>
#include <psp2/sysmodule.h>
#include <stdlib.h>
#include <malloc.h>
#include "log.h"
static bool preview_current(unsigned generation){lock();bool current=g_run && preview_generation==generation && preview_id[0];unlock();return current;}
static void preview_play(const char*id,unsigned generation){
 char path[128];FILE*f=NULL;
 const char*mounts[]={"ux0:patch","ux0:app","vs0:app"};
 for(unsigned i=0;i<3 && !f;i++){snprintf(path,sizeof(path),"%s/%s/sce_sys/snd0.at9",mounts[i],id);f=fopen(path,"rb");}
 if(!f)return;
 if(fseek(f,0,SEEK_END)){fclose(f);return;}long size=ftell(f);
 if(size<12 || size>8*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return;}
 unsigned char*data=memalign(256,(size+255)&~255);if(!data){fclose(f);return;}
 bool read_ok=fread(data,1,size,f)==(size_t)size;if(fclose(f))read_ok=false;
 if(!read_ok || memcmp(data,"RIFF",4) || memcmp(data+8,"WAVE",4) || !preview_current(generation)){free(data);return;}
 int loaded=sceSysmoduleLoadModule(SCE_SYSMODULE_ATRAC);bool module_owner=loaded>=0;
 if(loaded<0 && sceSysmoduleIsLoaded(SCE_SYSMODULE_ATRAC)<0){free(data);return;}
 SceAtracDecoderGroup group={sizeof(group),16,2};int needed=sceAtracQueryDecoderGroupMemSize(SCE_ATRAC_TYPE_AT9,&group);
 void*work=needed>0 && needed<=8*1024*1024?memalign(256,(needed+255)&~255):NULL;
 bool created=work && sceAtracCreateDecoderGroup(SCE_ATRAC_TYPE_AT9,&group,work,1)>=0;
 int handle=created?sceAtracSetDataAndAcquireHandle(data,size,size):-1;int port=-1;
 if(handle>=0){
  SceAtracContentInfo info;memset(&info,0,sizeof(info));info.size=sizeof(info);
  if(sceAtracGetContentInfo(handle,&info)>=0 && (info.channel==1 || info.channel==2) && (info.samplingRate==48000 || info.samplingRate==44100) && sceAtracSetOutputSamples(handle,2048)>=0){
   port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,2048,info.samplingRate,info.channel==2?SCE_AUDIO_OUT_MODE_STEREO:SCE_AUDIO_OUT_MODE_MONO);
   if(port>=0){
    short*pcm=memalign(256,2048*2*sizeof(short));int volume[2]={8192,8192};sceAudioOutSetVolume(port,3,volume);
    unsigned empty=0;
    while(pcm && preview_current(generation)){
     memset(pcm,0,2048*2*sizeof(short));SceUInt32 samples=0,status=0;int code=sceAtracDecode(handle,pcm,&samples,&status);
     if(code<0 || samples>2048)break;
     if(samples){empty=0;if(sceAudioOutOutput(port,pcm)<0)break;}else if(++empty>2)break;
     if(status&SCE_ATRAC_DECODER_STATUS_ALL_DATA_WAS_DECODED)if(sceAtracResetNextOutputPosition(handle,0)<0)break;
    }
    free(pcm);sceAudioOutReleasePort(port);
   }
  }
  sceAtracReleaseHandle(handle);
 }
 if(created)sceAtracDeleteDecoderGroup(SCE_ATRAC_TYPE_AT9,1);
 if(module_owner)sceSysmoduleUnloadModule(SCE_SYSMODULE_ATRAC);
 free(work);free(data);
 vs_log("game-audio",path,port<0?-1:0);
}
static int preview_worker(SceSize size,void*args){
 (void)size;(void)args;
 while(running()){
  sceKernelWaitSema(preview_sema,1,NULL);if(!running())break;
  while(sceKernelPollSema(preview_sema,1)>=0){}
  lock();unsigned generation=preview_generation;char id[10];memcpy(id,preview_id,10);unlock();if(!id[0])continue;
  sceKernelDelayThread(650000);if(!preview_current(generation))continue;
  while(preview_current(generation)){lock();bool boot=boot_playing;unlock();if(!boot)break;sceKernelDelayThread(50000);}
  if(preview_current(generation))preview_play(id,generation);
 }
 return 0;
}
