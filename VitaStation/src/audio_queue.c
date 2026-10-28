#include "audio_queue.h"
#include <string.h>
void audio_queue_init(AudioQueue*q){memset(q,0,sizeof(*q));q->enabled=true;}
bool audio_queue_push(AudioQueue*q,int e){
    if(q->stopped || !q->enabled || q->count==16 || e<0 || e>=5)return false;
    q->events[q->tail]=e;q->tail=(q->tail+1)%16;q->count++;return true;
}
bool audio_queue_pop(AudioQueue*q,int*e){
    if(!q->count || q->stopped)return false;
    *e=q->events[q->head];q->head=(q->head+1)%16;q->count--;return true;
}
void audio_queue_enable(AudioQueue*q,bool enabled){q->enabled=enabled;}
void audio_queue_stop(AudioQueue*q){q->stopped=true;q->count=0;}
