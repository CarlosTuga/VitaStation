#ifndef VS_AUDIO_SYNTH_H
#define VS_AUDIO_SYNTH_H
#include "audio.h"
#include <stdint.h>
#define VS_AUDIO_RATE 48000u
#define VS_SFX_MAX_SAMPLES 6144u
unsigned audio_synth_frames(SoundEffect effect);
int16_t audio_synth_sample(SoundEffect effect,unsigned frame);
#endif
