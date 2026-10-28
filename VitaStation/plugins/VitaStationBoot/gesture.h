#ifndef VS_PS_GESTURE_H
#define VS_PS_GESTURE_H
#include <stdint.h>
typedef struct {int down,taps,held;uint64_t pressed_at,released_at,last_time;} VitaStationPSGesture;
enum {VS_PS_NONE,VS_PS_SINGLE,VS_PS_DOUBLE};
static inline int vs_ps_gesture(VitaStationPSGesture*g,int down,uint64_t now){
    int event=VS_PS_NONE;
    if(now<g->last_time){g->down=g->taps=g->held=0;}
    g->last_time=now;
    if(g->down && now-g->pressed_at>=500000){g->held=1;g->taps=0;}
    if(down && !g->down){
        if(g->taps && now-g->released_at>350000){g->taps=0;event=VS_PS_SINGLE;}
        g->pressed_at=now;g->held=0;g->down=1;
    }else if(!down && g->down){
        g->down=0;
        if(!g->held){
            if(g->taps && now-g->released_at<=350000){g->taps=0;event=VS_PS_DOUBLE;}
            else {g->taps=1;g->released_at=now;}
        }
    }
    if(!g->down && g->taps && now-g->released_at>=350000){g->taps=0;event=VS_PS_SINGLE;}
    return event;
}
#endif
