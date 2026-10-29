#include "audio.h"
#include "audio_queue.h"
#include "audio_synth.h"
#include "audio_clip.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(__vita__)
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#define AUDIO_RATE      48000
#define AUDIO_GRAIN     256                 /* amostras por bloco (multiplo de 64) */
#define SFX_SAMPLES     VS_SFX_MAX_SAMPLES /* max. 128 ms; actual length depends on event */
#define SFX_COUNT       6

static short g_pcm[SFX_COUNT][SFX_SAMPLES * 2] __attribute__((aligned(64)));  /* estereo S16 */
static int g_port = -1;
static SceUID g_thread = -1;
static SceUID g_sema = -1;
static int g_run = 0;
static SceUID g_mutex=-1;
static AudioQueue queue;
static SceUID preview_thread=-1,preview_sema=-1;
static char preview_id[10];static unsigned preview_generation;static bool boot_playing;
static void lock(void){sceKernelLockMutex(g_mutex,1,NULL);}
static void unlock(void){sceKernelUnlockMutex(g_mutex,1);}
static int running(void){lock();int r=g_run;unlock();return r;}
#include "audio_preview.inc"
static bool play_startup(short*grain){
    (void)grain;AudioClip clip;if(!audio_clip_load("app0:assets/audio/startup.pcm",&clip))return false;
    if(sceAudioOutSetConfig(g_port,2048,AUDIO_RATE,SCE_AUDIO_OUT_MODE_STEREO)<0){audio_clip_free(&clip);return false;}
    int volume[2]={24576,24576};sceAudioOutSetVolume(g_port,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,volume);
    for(size_t frame=0;frame<clip.padded_frames && running();frame+=2048){
        lock();bool interrupted=queue.count>0;unlock();if(interrupted)break;
        if(sceAudioOutOutput(g_port,clip.samples+frame*2)<0)break;
    }
    sceAudioOutOutput(g_port,NULL);audio_clip_free(&clip);
    volume[0]=volume[1]=32768;sceAudioOutSetVolume(g_port,3,volume);sceAudioOutSetConfig(g_port,AUDIO_GRAIN,AUDIO_RATE,SCE_AUDIO_OUT_MODE_STEREO);return true;
}

/* Tocar um som NAO pode bloquear o thread da interface: sceAudioOutOutput espera pelo fim de cada
 * bloco, por isso os blocos sao enviados por uma thread propria. */
static int audio_thread(SceSize args, void *argp) {
    static short grain[AUDIO_GRAIN * 2] __attribute__((aligned(64)));
    (void)args; (void)argp;
    while (running()) {
        sceKernelWaitSema(g_sema, 1, NULL);
        lock();
        int req=-1,run=g_run;
        audio_queue_pop(&queue,&req);
        unlock();
        if (!run) break;
        if (req < 0 || req >= SFX_COUNT) continue;
        if(req==SND_BOOT){lock();boot_playing=true;unlock();bool played=play_startup(grain);lock();boot_playing=false;unlock();if(played)continue;}
        for (int pos = 0; pos < (int)audio_synth_frames((SoundEffect)req) && running(); pos += AUDIO_GRAIN) {
            if(req==SND_BOOT){for(unsigned i=0;i<AUDIO_GRAIN;i++){short sample=audio_synth_sample(SND_BOOT,pos+i);grain[2*i]=sample;grain[2*i+1]=(short)(sample*.94f);}}
            if(req==SND_BOOT)sceAudioOutOutput(g_port,grain);
            else sceAudioOutOutput(g_port,&g_pcm[req][pos*2]);
        }
    }
    return 0;
}
#endif

static bool g_sfx_enabled = true;
static bool g_music_enabled = false;   /* sem musica de fundo implementada */

void audio_init(void) {
#if defined(__vita__)
    if (g_port >= 0) return;
    for (int s = 0; s < SND_BOOT; s++) {
        for(unsigned i=0;i<SFX_SAMPLES;i++){
            short left=audio_synth_sample((SoundEffect)s,i);
            short right=(short)(left*.90f+(i>144?audio_synth_sample((SoundEffect)s,i-144)*.08f:0));
            g_pcm[s][i*2]=left;g_pcm[s][i*2+1]=right;
        }
    }
    g_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, AUDIO_GRAIN, AUDIO_RATE, SCE_AUDIO_OUT_MODE_STEREO);
    if (g_port < 0) return;                      /* sem audio: as outras funcoes passam a ser no-ops */
    audio_queue_init(&queue);
    g_mutex=sceKernelCreateMutex("vs_audio_lock",0,0,NULL);
    g_sema = sceKernelCreateSema("vs_audio_sema", 0, 0, 16, NULL);
    g_run = 1;
    g_thread = sceKernelCreateThread("vs_audio", audio_thread, 0x10000080, 0x4000, 0, 0, NULL);
    if (g_mutex < 0 || g_sema < 0 || g_thread < 0 || sceKernelStartThread(g_thread, 0, NULL) < 0) {
        g_run = 0;
        if (g_thread >= 0) { sceKernelDeleteThread(g_thread); g_thread = -1; }
        if (g_sema >= 0) { sceKernelDeleteSema(g_sema); g_sema = -1; }
        if(g_mutex>=0){sceKernelDeleteMutex(g_mutex);g_mutex=-1;}
        sceAudioOutReleasePort(g_port);
        g_port = -1;
    }
    if(g_port>=0){preview_sema=sceKernelCreateSema("vs-preview-wake",0,0,32,NULL);if(preview_sema>=0){preview_thread=sceKernelCreateThread("vs-game-preview",preview_worker,0x10000110,64*1024,0,0,NULL);if(preview_thread<0 || sceKernelStartThread(preview_thread,0,NULL)<0){if(preview_thread>=0)sceKernelDeleteThread(preview_thread);preview_thread=-1;sceKernelDeleteSema(preview_sema);preview_sema=-1;}}}
#else
    printf("[Audio Mock] Audio initialized\n");
#endif
}

void audio_play_sfx(SoundEffect sfx) {
    if (!g_sfx_enabled) return;
#if defined(__vita__)
    if (g_port < 0 || g_sema < 0 || (unsigned)sfx >= (unsigned)SFX_COUNT) return;
    lock();
    if(g_run && audio_queue_push(&queue,(int)sfx))sceKernelSignalSema(g_sema,1);
    unlock(); /* Full queue drops newest explicitly, never overwrites old requests. */
#else
    printf("[Audio SFX %d]\n", (int)sfx);
#endif
}

void audio_set_music_enabled(bool enable) {
    (void)enable;
    g_music_enabled = false;   /* nao ha musica implementada; a UI mostra "Unavailable" */
}

void audio_set_sfx_enabled(bool enable) {
    g_sfx_enabled = enable;
}

void audio_shutdown(void) {
#if defined(__vita__)
    if (g_port < 0) return;
    lock();g_run=0;audio_queue_stop(&queue);unlock();
    if(preview_sema>=0)sceKernelSignalSema(preview_sema,1);
    if(preview_thread>=0){sceKernelWaitThreadEnd(preview_thread,NULL,NULL);sceKernelDeleteThread(preview_thread);preview_thread=-1;}
    if(preview_sema>=0){sceKernelDeleteSema(preview_sema);preview_sema=-1;}
    if (g_sema >= 0) sceKernelSignalSema(g_sema, 1);
    if (g_thread >= 0) {
        sceKernelWaitThreadEnd(g_thread, NULL, NULL);
        sceKernelDeleteThread(g_thread);
        g_thread = -1;
    }
    if (g_sema >= 0) { sceKernelDeleteSema(g_sema); g_sema = -1; }
    sceKernelDeleteMutex(g_mutex);g_mutex=-1;
    sceAudioOutReleasePort(g_port);
    g_port = -1;
#endif
    (void)g_music_enabled;
}
void audio_preview_select(const char*id){
#if defined(__vita__)
 if(g_port<0 || preview_sema<0)return;
 char selected[10]={0};if(id){bool valid=strlen(id)==9;for(unsigned i=0;i<9 && valid;i++){unsigned char c=id[i];if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')))valid=false;}if(valid)memcpy(selected,id,9);}
 lock();bool changed=strcmp(selected,preview_id)!=0;if(changed){memcpy(preview_id,selected,10);preview_generation++;}unlock();if(changed)sceKernelSignalSema(preview_sema,1);
#else
 (void)id;
#endif
}
