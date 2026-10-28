#include "system_info.h"
#include <stdio.h>
#include <string.h>
#include "settings.h"
#if defined(__vita__)
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/power.h>
#include <psp2/io/devctl.h>
#include <psp2/io/dirent.h>
#include <psp2/rtc.h>
#include <psp2/ctrl.h>
#include <psp2/apputil.h>
#include <psp2/system_param.h>
#include <psp2/sysmodule.h>
#endif
#if defined(__vita__)
static int read_parameter(unsigned id,int*value){
    if(sceSysmoduleIsLoaded(SCE_SYSMODULE_APPUTIL)<0 && sceSysmoduleLoadModule(SCE_SYSMODULE_APPUTIL)<0)return -1;
    int result=sceAppUtilSystemParamGetInt(id,value);
    if((unsigned)result==SCE_APPUTIL_ERROR_NOT_INITIALIZED){SceAppUtilInitParam init={0};SceAppUtilBootParam boot={0};if(sceAppUtilInit(&init,&boot)<0)return result;result=sceAppUtilSystemParamGetInt(id,value);}
    return result;
}
#endif
size_t system_info_read(int page,char lines[6][128]){
    if(!lines)return 0;
    if(page==SYS_INFO_NETWORK)return system_network_describe(lines);
    if(page==SYS_INFO_LICENSES){
        snprintf(lines[0],128,"VitaStation - desenvolvido por Carlos Tuga");
        snprintf(lines[1],128,"Usagi PKGj / PKGj: BSD-2-Clause");
        snprintf(lines[2],128,"VitaShell: GPL-3.0-or-later (adaptacoes)");
        snprintf(lines[3],128,"Catalogo: PS Vita Alive e contribuidores");
        snprintf(lines[4],128,"Caratulas HexFlow: CC BY-NC-SA 4.0");
        snprintf(lines[5],128,"Licencas completas acompanham os pacotes.");return 6;
    }
    if(page==SYS_INFO_LOG){
        FILE*f=fopen(VS_DATA_DIR "/diagnostic.log","rb");if(!f)return 0;char line[512];size_t count=0;
        while(fgets(line,sizeof(line),f)){if(count==6){memmove(lines,lines+1,5*128);count=5;}line[strcspn(line,"\r\n")]=0;snprintf(lines[count++],128,"%.127s",line);}
        fclose(f);return count;
    }
#if defined(__vita__)
    size_t count=0;
    if(page==SYS_INFO_CONSOLE){
        SceKernelSystemSwVersion fw={0};fw.size=sizeof(fw);
        if(sceKernelGetSystemSwVersion(&fw)>=0){snprintf(lines[count++],128,"Versao reportada: %.27s",fw.versionString);snprintf(lines[count++],128,"Codigo de versao: 0x%08X",(unsigned)fw.version);}
        int model=sceKernelGetModel();if(model>=0)snprintf(lines[count++],128,"Identificador do modelo: %d",model);
    }else if(page==SYS_INFO_BATTERY){
        int level=scePowerGetBatteryLifePercent();if(level>=0 && level<=100)snprintf(lines[count++],128,"Carga da bateria: %d%%",level);
        int charging=scePowerIsBatteryCharging();if(charging>=0)snprintf(lines[count++],128,"A carregar: %s",charging?"Sim":"Nao");
        int minutes=scePowerGetBatteryLifeTime();if(minutes>=0)snprintf(lines[count++],128,"Autonomia estimada: %d minutos",minutes);
    }else if(page==SYS_INFO_STORAGE){
        const char*devices[]={"ux0:","ur0:"};for(unsigned i=0;i<2;i++){SceIoDevInfo info={0};if(sceIoDevctl(devices[i],0x3001,NULL,0,&info,sizeof(info))>=0){snprintf(lines[count++],128,"%s Livre: %.1f MiB / Total: %.1f MiB",devices[i],(double)info.free_size/1048576.0,(double)info.max_size/1048576.0);}}
    }else if(page==SYS_INFO_CLOCK){
        SceDateTime time;if(sceRtcGetCurrentClockLocalTime(&time)>=0)snprintf(lines[count++],128,"Hora local: %04u-%02u-%02u %02u:%02u:%02u",time.year,time.month,time.day,time.hour,time.minute,time.second);
        int value;
        if(read_parameter(SCE_SYSTEM_PARAM_ID_TIME_ZONE,&value)>=0)snprintf(lines[count++],128,"Fuso horario configurado: UTC %+d minutos",value);
        if(read_parameter(SCE_SYSTEM_PARAM_ID_DAYLIGHT_SAVINGS,&value)>=0)snprintf(lines[count++],128,"Hora de verao: %s",value?"Ativada":"Desativada");
        if(read_parameter(SCE_SYSTEM_PARAM_ID_DATE_FORMAT,&value)>=0){const char*formats[]={"Ano/Mes/Dia","Dia/Mes/Ano","Mes/Dia/Ano"};snprintf(lines[count++],128,"Formato da data: %s",value>=0 && value<3?formats[value]:"Desconhecido");}
        if(read_parameter(SCE_SYSTEM_PARAM_ID_TIME_FORMAT,&value)>=0)snprintf(lines[count++],128,"Formato da hora: %s",value==SCE_SYSTEM_PARAM_TIME_FORMAT_24HR?"24 horas":"12 horas");
        snprintf(lines[count++],128,"Consulta real; edicao ainda indisponivel.");
    }else if(page==SYS_INFO_LANGUAGE){
        int value;const char*languages[]={"Japones","Ingles (EUA)","Frances","Espanhol","Alemao","Italiano","Neerlandes","Portugues (Portugal)","Russo","Coreano","Chines tradicional","Chines simplificado","Finlandes","Sueco","Dinamarques","Noruegues","Polaco","Portugues (Brasil)","Ingles (Reino Unido)","Turco"};
        if(read_parameter(SCE_SYSTEM_PARAM_ID_LANG,&value)>=0)snprintf(lines[count++],128,"Idioma da consola: %s",value>=0 && value<20?languages[value]:"Desconhecido");
        if(read_parameter(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON,&value)>=0)snprintf(lines[count++],128,"Botao de confirmacao do sistema: %s",value==SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS?"X":"Circulo");
        snprintf(lines[count++],128,"Consulta real; edicao global ainda indisponivel.");
    }else if(page==SYS_INFO_INPUT){
        SceCtrlData pad;if(sceCtrlPeekBufferPositive(0,&pad,1)>0){snprintf(lines[count++],128,"Botoes: 0x%08X",(unsigned)pad.buttons);snprintf(lines[count++],128,"Analogico esquerdo: X=%u Y=%u",pad.lx,pad.ly);snprintf(lines[count++],128,"Analogico direito: X=%u Y=%u",pad.rx,pad.ry);}
    }else if(page==SYS_INFO_PERFORMANCE){
        snprintf(lines[count++],128,"CPU: %d MHz | GPU: %d MHz",scePowerGetArmClockFrequency(),scePowerGetGpuClockFrequency());
        snprintf(lines[count++],128,"BUS: %d MHz | GPU XBAR: %d MHz",scePowerGetBusClockFrequency(),scePowerGetGpuXbarClockFrequency());
        snprintf(lines[count++],128,"Temperatura bateria: %d | Ciclos: %d",scePowerGetBatteryTemp(),scePowerGetBatteryCycleCount());
        snprintf(lines[count++],128,"Saude da bateria: %d%%",scePowerGetBatterySOH());
    }else if(page==SYS_INFO_DOWNLOADS){
        SceUID dir=sceIoDopen("ux0:bgdl/t");if(dir<0)return 0;SceIoDirent entry;unsigned jobs=0;
        while(memset(&entry,0,sizeof(entry)),sceIoDread(dir,&entry)>0){if(entry.d_name[0]=='.')continue;jobs++;if(count<4)snprintf(lines[count++],128,"Tarefa do sistema: %.80s",entry.d_name);}
        sceIoDclose(dir);snprintf(lines[count++],128,"Total de tarefas do sistema: %u",jobs);snprintf(lines[count++],128,"Progresso e cancelamento: notificacoes LiveArea.");
    }return count;
#else
    (void)page;return 0;
#endif
}
