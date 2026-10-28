#include "system_controls.h"
#include "log.h"
#if defined(__vita__)
#include <psp2/avconfig.h>
#include <psp2/power.h>
#include <psp2/registrymgr.h>
#endif

SystemControlResult system_control_apply(SystemControl which,int value,const SystemControlBackend*b){
    SystemControlResult r={SYS_INVALID,0,-1};
    if((which!=SYS_VOLUME && which!=SYS_BRIGHTNESS) || value<0 || value>(which==SYS_VOLUME?30:100) || (which==SYS_BRIGHTNESS && value<5))return r;
    r.status=SYS_UNAVAILABLE;if(!b)return r;
    if(which==SYS_VOLUME){
        if(!b->read_volume || !b->write_volume)return r;
        int before=-1;r.code=b->read_volume(&before);
        if(r.code<0 || before<0 || before>30){r.status=SYS_READ_FAILED;return r;}
        r.code=b->write_volume(value);if(r.code<0){r.status=SYS_WRITE_FAILED;return r;}
        int after=-1;r.code=b->read_volume(&after);
        if(r.code<0 || after!=value){r.status=SYS_VERIFY_FAILED;return r;}
        r.value=after;r.status=SYS_APPLIED;
    }else{
        if(!b->max_brightness || !b->write_brightness)return r;
        int maximum=0;r.code=b->max_brightness(&maximum);
        if(r.code<0 || maximum<21 || maximum>65536){r.status=SYS_READ_FAILED;return r;}
        int requested=(maximum*value+50)/100;if(requested<21)requested=21;
        int before=-1;
        if(b->read_brightness && b->save_brightness){
            r.code=b->read_brightness(&before);if(r.code<0 || before<21 || before>maximum){r.status=SYS_READ_FAILED;return r;}
        }
        r.code=b->write_brightness(requested);if(r.code<0){r.status=SYS_WRITE_FAILED;return r;}
        if(b->read_brightness && b->save_brightness){
            r.code=b->save_brightness(requested);
            if(r.code<0){b->write_brightness(before);r.status=SYS_WRITE_FAILED;return r;}
            int after=-1;r.code=b->read_brightness(&after);
            if(r.code<0 || after!=requested){r.status=SYS_VERIFY_FAILED;return r;}
            r.value=(after*100+maximum/2)/maximum;r.status=SYS_BRIGHTNESS_SAVED;return r;
        }
        /* SDK exposes no current-brightness getter. Do not call this verified or persisted. */
        r.value=value;r.status=SYS_APPLIED_UNVERIFIED;
    }
    return r;
}
#if defined(__vita__)
static int read_brightness(int*value){return sceRegMgrGetKeyInt("/CONFIG/DISPLAY","brightness",value);}
static int save_brightness(int value){return sceRegMgrSetKeyInt("/CONFIG/DISPLAY","brightness",value);}
#endif
SystemControlResult system_control_set(SystemControl which,int value){
#if defined(__vita__)
    const SystemControlBackend b={sceAVConfigGetSystemVol,sceAVConfigSetSystemVol,sceAVConfigGetDisplayMaxBrightness,sceAVConfigSetDisplayBrightness,read_brightness,save_brightness};
    SystemControlResult r=system_control_apply(which,value,&b);
#else
    SystemControlResult r=system_control_apply(which,value,NULL);
#endif
    if(r.status!=SYS_APPLIED && r.status!=SYS_APPLIED_UNVERIFIED && r.status!=SYS_BRIGHTNESS_SAVED)vs_log("system-control",system_control_message(r.status),r.code);
    return r;
}
SystemControlResult system_control_read(SystemControl which){
    SystemControlResult r={SYS_UNAVAILABLE,0,-1};
#if defined(__vita__)
    int maximum=0,value=-1;
    if(which==SYS_VOLUME){r.code=sceAVConfigGetSystemVol(&value);if(r.code>=0 && value>=0 && value<=30){r.value=value;r.status=SYS_APPLIED;}else r.status=SYS_READ_FAILED;}
    else if(which==SYS_BRIGHTNESS){r.code=sceAVConfigGetDisplayMaxBrightness(&maximum);if(r.code>=0)r.code=read_brightness(&value);
        if(r.code>=0 && maximum>=21 && maximum<=65536 && value>=21 && value<=maximum){r.value=(value*100+maximum/2)/maximum;r.status=SYS_BRIGHTNESS_SAVED;}else r.status=SYS_READ_FAILED;}
#else
    (void)which;
#endif
    return r;
}
const char *system_control_message(SystemControlStatus s){
    switch(s){
        case SYS_APPLIED:return "Alteracao aplicada e confirmada pela consola.";
        case SYS_APPLIED_UNVERIFIED:return "Brilho enviado. Sem leitura de confirmacao no SDK.";
        case SYS_BRIGHTNESS_SAVED:return "Brilho aplicado e definicao do sistema confirmada.";
        case SYS_UNAVAILABLE:return "Este controlo exige uma PS Vita compativel.";
        case SYS_READ_FAILED:return "Nao foi possivel ler o estado da consola.";
        case SYS_WRITE_FAILED:return "A consola recusou a alteracao.";
        case SYS_VERIFY_FAILED:return "Nao foi possivel confirmar o novo valor.";
        default:return "Valor fora dos limites permitidos.";
    }
}
int system_power_request(SystemPowerAction action){
    if(action!=SYS_POWER_OFF && action!=SYS_POWER_RESTART)return -1;
#if defined(__vita__)
    int result=action==SYS_POWER_OFF?scePowerRequestStandby():scePowerRequestColdReset();
    if(result<0)vs_log("power","system power request rejected",result);
    return result;
#else
    return -1;
#endif
}

int system_suspend_request(void){
#if defined(__vita__)
 return scePowerRequestSuspend();
#else
 return -1;
#endif
}
