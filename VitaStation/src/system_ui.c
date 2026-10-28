#include "maintenance.h"
#include "updater.h"
#include "version.h"
#include "system_ui.h"
#include "settings_menu.h"
#include "system_controls.h"
#include "system_info.h"
#include "autoboot.h"
#include "settings.h"
#include "store.h"
#include "render.h"
#include <stdio.h>
#include <string.h>
void ui_open_power_menu(UIContext*u){if(!u || u->state==UI_STATE_POWER)return;u->power_return_state=u->state;u->state=UI_STATE_POWER;u->power_index=2;}
void system_ui_update(UIContext*u,float dt){
    if(!u || u->state!=UI_STATE_SYSTEM_INFO)return;
    u->information_poll-=dt;if(u->information_poll<=0){u->information_poll=1;u->information_count=system_info_read(u->information_page,u->information_lines);}
}
bool system_ui_input(UIContext*u,Library*lib,SystemUICommand command){
    if(!u)return false;
    if(settings_menu_input(u,command))return true;
    if(u->state==UI_STATE_SETTINGS && command==SYSTEM_UI_CONFIRM){
        if(u->settings_index>=UI_LOCAL_SETTINGS_COUNT){
            const SystemFeature*f=system_feature_get((size_t)(u->settings_index-UI_LOCAL_SETTINGS_COUNT));
            if(f && f->control>=SYS_ACTION_LIBRARY){
                switch(f->control){
                    case SYS_ACTION_LIBRARY:ui_select_tab(u,SECTION_GAMES);u->state=UI_STATE_HOME;return true;
                    case SYS_ACTION_STORE:ui_select_tab(u,SECTION_STORE);u->state=UI_STATE_HOME;u->catalog_tabs=true;u->catalog_load_requested=true;return true;
                    case SYS_ACTION_PLUGINS:u->settings_index=5;ui_confirm(u,lib);return true;
                    case SYS_ACTION_PLUGIN_PREPARE:plugins_prepare(u->plugins_message,sizeof(u->plugins_message));ui_show_toast(u,u->plugins_message);return true;
                    case SYS_ACTION_POWER:ui_open_power_menu(u);return true;
                    case SYS_ACTION_AUTOBOOT:u->settings_index=4;return true;
                    case SYS_ACTION_PS:u->settings_index=6;return true;
                    case SYS_ACTION_PREFERENCES:u->settings_index=0;return true;
                    case SYS_ACTION_QUICK:ui_open_quick_menu(u);return true;
                    case SYS_ACTION_REFRESH:u->rescan_requested=true;u->state=UI_STATE_HOME;return true;
                    case SYS_ACTION_SUSPEND:if(store_busy())ui_show_toast(u,"Instalador local ativo; aguarda a conclusao.");else ui_show_toast(u,system_suspend_request()<0?"A consola recusou suspender.":"Pedido para suspender enviado.");return true;
                }return true;
            }
            if(f && f->control>=SYS_INFO_NETWORK){u->information_page=f->control;u->information_count=system_info_read(u->information_page,u->information_lines);u->information_poll=1;u->state=UI_STATE_SYSTEM_INFO;return true;}
        }
        if(u->settings_index==14){u->power_index=0;u->state=UI_STATE_UNINSTALL;return true;}
        if(u->settings_index==12){ui_show_toast(u,"VitaStation " VS_VERSION_LABEL);return true;}
        if(u->settings_index==13){store_update_request();return true;}
        if(u->settings_index==5){u->plugin_count=plugins_list(u->plugin_entries,96);u->plugin_index=0;u->state=UI_STATE_PLUGINS;return true;}
        if(u->settings_index==6){
            if(!autoboot_bridge_available()){ui_show_toast(u,"O plugin do VitaStation ainda nao esta configurado.");return true;}
            bool before=u->ps_routing_enabled;u->ps_routing_enabled=!before;
            if(!settings_save(u,VS_CONFIG_PATH) || !autoboot_marker_write(VS_DATA_DIR "/ps-routing.enabled",u->ps_routing_enabled)){
                u->ps_routing_enabled=before;settings_save(u,VS_CONFIG_PATH);autoboot_marker_write(VS_DATA_DIR "/ps-routing.enabled",before);ui_show_toast(u,"Nao foi possivel guardar o comportamento do botao PS.");
            }else ui_show_toast(u,u->ps_routing_enabled?"Botao PS configurado. Reinicia para carregar o plugin.":"Encaminhamento PS desligado.");
            return true;
        }
        if(u->settings_index==7){ui_open_power_menu(u);return true;}
        if(u->settings_index==8){
            bool before=u->auto_dlcs_enabled;u->auto_dlcs_enabled=!before;
            if(!settings_save(u,VS_CONFIG_PATH)){u->auto_dlcs_enabled=before;ui_show_toast(u,"Nao foi possivel guardar a opcao de DLCs.");}
            else ui_show_toast(u,u->auto_dlcs_enabled?"DLCs da mesma ID: descarga e instalacao automaticas.":"DLCs automaticos desligados.");
            return true;
        }
    }
    if(u->state==UI_STATE_UNINSTALL){
        if(command==SYSTEM_UI_CANCEL){u->state=UI_STATE_SETTINGS;return true;}
        if(command==SYSTEM_UI_LEFT || command==SYSTEM_UI_UP)u->power_index=0;
        if(command==SYSTEM_UI_RIGHT || command==SYSTEM_UI_DOWN)u->power_index=1;
        if(command==SYSTEM_UI_CONFIRM){if(!u->power_index)u->state=UI_STATE_SETTINGS;else{
            if(store_busy()){ui_show_toast(u,"Aguarda as instalacoes antes de remover.");return true;}
            bool before_boot=u->autoboot_enabled,before_ps=u->ps_routing_enabled;u->autoboot_enabled=false;u->ps_routing_enabled=false;
            if(!settings_save(u,VS_CONFIG_PATH)){u->autoboot_enabled=before_boot;u->ps_routing_enabled=before_ps;ui_show_toast(u,"Nao foi possivel guardar a desativacao. Remocao cancelada.");return true;}
            char error[256];maintenance_uninstall(error,sizeof(error));u->autoboot_enabled=autoboot_is_enabled();u->ps_routing_enabled=autoboot_marker_read(VS_DATA_DIR "/ps-routing.enabled");settings_save(u,VS_CONFIG_PATH);ui_show_toast(u,error);
        }}
        return true;
    }
    if(u->state==UI_STATE_SYSTEM_INFO){
        if(command==SYSTEM_UI_CANCEL)u->state=UI_STATE_SETTINGS;
        if(command==SYSTEM_UI_CONFIRM){u->information_count=system_info_read(u->information_page,u->information_lines);u->information_poll=1;}
        return true;
    }
    if(u->state==UI_STATE_POWER){
        if(command==SYSTEM_UI_CANCEL){u->state=u->power_return_state;return true;}
        if(command==SYSTEM_UI_LEFT || command==SYSTEM_UI_UP){if(u->power_index>0)--u->power_index;return true;}
        if(command==SYSTEM_UI_RIGHT || command==SYSTEM_UI_DOWN){if(u->power_index<2)++u->power_index;return true;}
        if(command==SYSTEM_UI_CONFIRM){
            if(u->power_index==2){u->state=u->power_return_state;return true;}
            if(store_busy()){ui_show_toast(u,"Aguarda a conclusao das instalacoes antes de reiniciar ou desligar.");return true;}
            if(!settings_save(u,VS_CONFIG_PATH) || (lib && !library_save_userdata(lib,VS_USERDATA_PATH))){ui_show_toast(u,"Nao foi possivel guardar os dados. Operacao cancelada.");return true;}
            int result=system_power_request(u->power_index==0?SYS_POWER_OFF:SYS_POWER_RESTART);
            ui_show_toast(u,result<0?"A consola nao aceitou o pedido de energia.":u->power_index==0?"Pedido para desligar enviado.":"Pedido para reiniciar enviado.");return true;
        }return true;
    }
    if(u->state==UI_STATE_PLUGINS){
        if(command==SYSTEM_UI_CANCEL){u->state=UI_STATE_SETTINGS;return true;}
        if(command==SYSTEM_UI_LEFT || command==SYSTEM_UI_UP){if(u->plugin_index>0)--u->plugin_index;return true;}
        if(command==SYSTEM_UI_RIGHT || command==SYSTEM_UI_DOWN){if((size_t)(u->plugin_index+1)<u->plugin_count)++u->plugin_index;return true;}
        if(command==SYSTEM_UI_CONFIRM && u->plugin_count){
            PluginEntry*entry=&u->plugin_entries[u->plugin_index];char message[256];
            if(plugins_set_enabled(entry,!entry->enabled,message,sizeof(message))){entry->enabled=!entry->enabled;}
            ui_show_toast(u,message);
        }return true;
    }
    return false;
}
bool system_ui_touch(UIContext*u,Library*lib,int x,int y){
    if(!u)return false;
    if(settings_menu_touch(u,lib,x,y))return true;
    if(u->state==UI_STATE_UNINSTALL){if(y>=340 && y<410 && x>=100 && x<860){u->power_index=x>=500;system_ui_input(u,lib,SYSTEM_UI_CONFIRM);}return true;}
    if(u->state==UI_STATE_SYSTEM_INFO){if(y>=410 && y<470)system_ui_input(u,lib,SYSTEM_UI_CONFIRM);return true;}
    if(u->state==UI_STATE_POWER){if(y>=280 && y<360 && x>=80 && x<880){u->power_index=(x-80)/270;if(u->power_index>2)u->power_index=2;system_ui_input(u,lib,SYSTEM_UI_CONFIRM);}return true;}
    if(u->state==UI_STATE_PLUGINS){int first=u->plugin_index>5?u->plugin_index-5:0;
        if(x>=40 && x<920 && y>=130 && y<466){size_t index=(size_t)(first+(y-130)/48);if(index<u->plugin_count){if((int)index==u->plugin_index)system_ui_input(u,lib,SYSTEM_UI_CONFIRM);else u->plugin_index=(int)index;}}return true;}
    return false;
}
static void clipped(int x,int y,const char*text,size_t limit,float scale,uint32_t color){
    char out[128];size_t n=strlen(text);if(n>limit)n=limit;while(n && (((unsigned char)text[n]&0xc0)==0x80))--n;memcpy(out,text,n);out[n]=0;if(strlen(text)>n)strcat(out,"...");render_draw_text(x,y,color,scale,out);
}
bool system_ui_render(UIContext*u){
    if(u->state==UI_STATE_UNINSTALL){
        render_fill_rect(0,60,960,444,0xFF111827);
        render_draw_text(60,135,COLOR_WHITE,1.4f,"Desinstalar VitaStation e caratulas?");
        render_draw_text(60,195,0xFFB8C7DC,.85f,"Remove o launcher, o instalador e todas as caratulas.");
        render_draw_text(60,235,0xFFB8C7DC,.85f,"Desativa o arranque automatico e os atalhos PS / Power.");
        render_draw_text(60,275,0xFFB8C7DC,.85f,"Jogos, Adrenaline e outros plugins sao preservados.");
        for(int i=0;i<2;i++){bool selected=u->power_index==i;render_fill_rounded_rect(100+i*400,340,360,70,16,selected?0xFFF2F5FA:0xFF26364D);render_draw_text(135+i*400,385,selected?0xFF172136:COLOR_WHITE,1.0f,i?"Desinstalar tudo":"Cancelar");}
        render_draw_text(60,465,0xFFB8C7DC,.8f,"Esquerda / Direita: escolher    X: confirmar    O: cancelar");return true;
    }
    if(u->state==UI_STATE_SYSTEM_INFO){
        render_fill_rect(0,60,960,444,0xFF111827);
        const char*title=u->information_page==SYS_INFO_NETWORK?"Estado da ligacao":u->information_page==SYS_INFO_CONSOLE?"Informacoes da consola":u->information_page==SYS_INFO_STORAGE?"Armazenamento":u->information_page==SYS_INFO_CLOCK?"Relogio da consola":u->information_page==SYS_INFO_LOG?"Registo de erros":u->information_page==SYS_INFO_INPUT?"Diagnostico do comando":u->information_page==SYS_INFO_LICENSES?"Creditos e licencas":u->information_page==SYS_INFO_PERFORMANCE?"Estado do sistema":u->information_page==SYS_INFO_DOWNLOADS?"Descargas LiveArea":u->information_page==SYS_INFO_LANGUAGE?"Idioma e confirmacao":"Bateria";
        render_draw_text(50,115,COLOR_WHITE,1.5f,title);render_draw_text(50,156,0xFFB8C7DC,.8f,"Consulta dos valores atuais da consola");
        if(!u->information_count)render_draw_text(50,240,COLOR_WHITE,1.1f,"Op\xc3\xa7\xc3\xa3o indispon\xc3\xadvel");
        for(size_t i=0;i<u->information_count && i<6;i++)clipped(50,207+(int)i*39,u->information_lines[i],78,.85f,COLOR_WHITE);
        render_draw_text(50,475,0xFFB8C7DC,.8f,"X: atualizar    O: voltar    Apenas leitura");return true;
    }
    if(u->state==UI_STATE_POWER){
        render_fill_rect(0,60,960,444,0xF30D1423);render_draw_text(80,165,COLOR_WHITE,1.8f,"Energia");
        render_draw_text(80,215,0xFFB8C7DC,.95f,"Escolhe uma opcao para a tua PS Vita.");
        const char*names[]={"Desligar","Reiniciar","Cancelar"};
        for(int i=0;i<3;i++){bool selected=i==u->power_index;int x=80+i*270;render_fill_rounded_rect(x,280,250,76,20,selected?0xFFF2F5FA:0xFF26364D);render_draw_text(x+40,327,selected?0xFF172136:COLOR_WHITE,1.15f,names[i]);}
        render_draw_text(80,415,0xFFB8C7DC,.85f,"X: confirmar    O: cancelar");return true;
    }
    if(u->state==UI_STATE_PLUGINS){
        render_fill_rect(0,60,960,444,0xFF111827);render_draw_text(40,106,COLOR_WHITE,1.6f,"Plugins");
        render_draw_text(460,102,0xFFB8C7DC,.75f,"Configuracao para o proximo reinicio");
        if(!u->plugin_count){render_draw_text(40,240,COLOR_WHITE,1,"Nao foi possivel ler a configuracao de plugins.");return true;}
        int first=u->plugin_index>5?u->plugin_index-5:0;
        for(int i=first;i<first+7 && (size_t)i<u->plugin_count;i++){
            PluginEntry*e=&u->plugin_entries[i];const char*name=strrchr(e->path,'/');name=name?name+1:e->path;int y=159+(i-first)*48;bool selected=i==u->plugin_index;
            if(selected)render_fill_rounded_rect(36,y-28,880,42,8,0xFFF2F5FA);
            uint32_t color=selected?0xFF172136:COLOR_WHITE;clipped(50,y,name,44,.85f,color);render_draw_text(530,y,color,.7f,e->section);
            render_draw_text(685,y,color,.8f,!e->enabled?"Desativado":!e->present?"Ficheiro em falta":e->protected_entry?"Essencial":"Ativado");
        }
        render_draw_text(40,492,0xFFB8C7DC,.7f,"X: ativar / desativar    O: voltar    As alteracoes exigem reinicio.");return true;
    }
    return false;
}
