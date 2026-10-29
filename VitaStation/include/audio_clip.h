#ifndef VS_AUDIO_CLIP_H
#define VS_AUDIO_CLIP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {void*storage;int16_t*samples;size_t frames,padded_frames;} AudioClip;
bool audio_clip_load(const char*path,AudioClip*clip);
void audio_clip_free(AudioClip*clip);
#endif
