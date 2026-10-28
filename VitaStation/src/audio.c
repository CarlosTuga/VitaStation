#include "audio.h"
#include "audio_queue.h"
#include "audio_synth.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(__vita__)
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#define AUDIO_RATE      48000
#define AUDIO_GRAIN     256                 /* amostras por bloco (multiplo de 64) */
#define SFX_SAMPLES     VS_SFX_MAX_SAMPLES /* max. 128 ms; actual length depends on event */
#define SFX_COUNT       5

static short g_pcm[SFX_COUNT][SFX_SAMPLES * 2] __attribute__((aligned(64)));  /* estereo S16 */
static int g_port = -1;
static SceUID g_thread = -1;
static SceUID g_sema = -1;
static int g_run = 0;
static SceUID g_mutex=-1;
static AudioQueue queue;
static void lock(void){sceKernelLockMutex(g_mutex,1,NULL);}
static void unlock(void){sceKernelUnlockMutex(g_mutex,1);}
static int running(void){lock();int r=g_run;unlock();return r;}

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
        for (int pos = 0; pos < (int)audio_synth_frames((SoundEffect)req) && running(); pos += AUDIO_GRAIN) {
            memcpy(grain, &g_pcm[req][pos * 2], sizeof(grain));
            sceAudioOutOutput(g_port, grain);
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
    for (int s = 0; s < SFX_COUNT; s++) {
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
    g_thread = sceKernelCreateThread("vs_audio", audio_thread, 0x10000100, 0x4000, 0, 0, NULL);
    if (g_mutex < 0 || g_sema < 0 || g_thread < 0 || sceKernelStartThread(g_thread, 0, NULL) < 0) {
        g_run = 0;
        if (g_thread >= 0) { sceKernelDeleteThread(g_thread); g_thread = -1; }
        if (g_sema >= 0) { sceKernelDeleteSema(g_sema); g_sema = -1; }
        if(g_mutex>=0){sceKernelDeleteMutex(g_mutex);g_mutex=-1;}
        sceAudioOutReleasePort(g_port);
        g_port = -1;
    }
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
