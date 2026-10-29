#ifndef VS_PS_SHORTCUT_H
#define VS_PS_SHORTCUT_H
#include "gesture.h"
enum {VS_PS_LIVEAREA=3};
typedef struct {VitaStationPSGesture ps;int latched,left;uint64_t blocked_until;} VitaStationPSRoute;
static inline int vs_ps_route(VitaStationPSRoute*r,int ps,int left,uint64_t now,int normal){
 r->left=left;
 if(ps && left && !r->latched){r->latched=1;r->ps=(VitaStationPSGesture){0};r->blocked_until=now+1500000;return VS_PS_LIVEAREA;}
 if(r->latched){r->ps=(VitaStationPSGesture){0};if(!ps && !left)r->latched=0;return VS_PS_NONE;}
 if(now<r->blocked_until || !normal){r->ps=(VitaStationPSGesture){0};return VS_PS_NONE;}
 return vs_ps_gesture(&r->ps,ps,now);
}
#endif
