#ifndef VS_AUDIO_QUEUE_H
#define VS_AUDIO_QUEUE_H
#include <stdbool.h>
/* Caller owns synchronization. Single consumer on Vita, protected by audio mutex. */
typedef struct { int events[16]; unsigned head,tail,count; bool enabled,stopped; } AudioQueue;
void audio_queue_init(AudioQueue *q);
bool audio_queue_push(AudioQueue *q,int event);
bool audio_queue_pop(AudioQueue *q,int *event);
void audio_queue_enable(AudioQueue *q,bool enabled);
void audio_queue_stop(AudioQueue *q);
#endif
