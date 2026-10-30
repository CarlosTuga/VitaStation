#include "integrations.h"
#include "game_support.h"
#include "cover_pack.h"
#include "updater.h"
#include "version.h"
#include "ui.h"
#include "store_ui.h"
#include "store.h"
#include "system_controls.h"
#include "autoboot.h"
#include "system_ui.h"
#include "settings_menu.h"
#include "psp_media.h"
#include "theme.h"
#include "personalization.h"
#include "bootstrap.h"
#include "system_info.h"
#include "render.h"
#include "launcher.h"
#include "audio.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "settings.h"
#include "log.h"

#if defined(__vita__)
#include <psp2/power.h>
#include <psp2/avconfig.h>
#include <psp2/rtc.h>
#endif

#define CARD_BASE_WIDTH  64
#define CARD_BASE_HEIGHT 64
#define CARD_SELECTED_WIDTH  108
#define CARD_SELECTED_HEIGHT 108
#define CARD_GAP 18
#define CAROUSEL_Y 100

static float carousel_offset(const UIContext *ui,int index){
    float x=0;for(int i=0;i<index;i++)x+=CARD_BASE_WIDTH*ui->card_scales[i]+CARD_GAP;return x;
}
static void draw_settings_symbol(int x,int y,uint32_t color){
    for(int i=0;i<3;i++){
        render_fill_rect(x,y+i*6,18,1,color);
        render_fill_rounded_rect(x+(i==1?10:4),y+i*6-2,4,5,2,color);
    }
}
static void draw_more_symbol(int x,int y,uint32_t color){
    for(int i=0;i<3;i++)render_fill_rounded_rect(x+i*6,y,3,3,1,color);
}
static void draw_star(int x,int y,uint32_t color){
    /* A small outline made from native primitives, not an image asset. */
    static const int pts[10][2]={{10,0},{13,7},{20,7},{15,12},{17,20},{10,15},{3,20},{5,12},{0,7},{7,7}};
    for(int i=0;i<10;i++)render_draw_line(x+pts[i][0],y+pts[i][1],x+pts[(i+1)%10][0],y+pts[(i+1)%10][1],color,1);
}

/* Keep UTF-8 boundaries when shortening metadata to the available layout. */
static void draw_title(int x,int y,float scale,const char *title,size_t limit){
    char text[128];size_t n=strlen(title);
    if(n>limit){n=limit;while(n && (((unsigned char)title[n]&0xC0)==0x80))n--;memcpy(text,title,n);memcpy(text+n,"...",4);}
    else snprintf(text,sizeof(text),"%s",title);
    render_draw_text(x,y,COLOR_WHITE,scale,text);
}

void ui_init(UIContext *ui) {
    if (!ui) return;
    memset(ui, 0, sizeof(UIContext));
    ui->state = UI_STATE_HOME;
    ui->current_section = SECTION_GAMES;
    ui->selected_card_index = 0;
    ui->details_button_index = 0;
    ui->scroll_offset = 0.0f;
    ui->target_scroll_offset = 0.0f;
    ui->bg_crossfade_alpha = 1.0f;

    for (int i = 0; i < MAX_APPS; i++) {
        ui->card_scales[i] = 1.0f;
    }

    ui->clock_str[0] = '\0';
    ui->battery_percent = 100;
    ui->system_volume=-1;ui->brightness_request=-1;ui->ps_routing_enabled=true;ui->auto_dlcs_enabled=true;ui->power_index=2;
    ui->sound_enabled = true;
    ui->music_enabled = false;
    ui->animations_enabled = true;
    ui->high_contrast=true;

    ui_show_toast(ui, "VitaStation");
}

void ui_show_toast(UIContext *ui, const char *msg) {
    if (!ui || !msg) return;
    snprintf(ui->toast_msg, sizeof(ui->toast_msg), "%.127s", msg);
    ui->toast_timer = 3.0f;
}

void ui_select_tab(UIContext *ui, AppSection section) {
    if(!ui || (unsigned)section>=(unsigned)SECTION_COUNT)return;
    if(ui->current_section!=section){
        ui->current_section=section;ui->selected_card_index=0;
        ui->scroll_offset=ui->target_scroll_offset=0;
        ui->details_button_index=0;
        ui->store_action_index=0;
        if(section==SECTION_STORE && ui->catalog)ui->catalog_load_requested=true;
    }
}
bool ui_handle_category_buttons(UIContext *ui,uint32_t pressed){
    if(!ui || ui->state==UI_STATE_PERSONALIZATION || ui->state==UI_STATE_ASSET_PICKER || ui->state==UI_STATE_SYSTEM_APPS || ui->state==UI_STATE_PLUGIN_CATALOG || ui->state==UI_STATE_REMOVE_GAME || ui->state==UI_STATE_GAME_SUPPORT || ui->state==UI_STATE_UNINSTALL || ui->state==UI_STATE_POWER || ui->state==UI_STATE_PLUGINS || ui->state==UI_STATE_SYSTEM_INFO || ui->state==UI_STATE_LOADING || ui->state==UI_STATE_SPLASH)return false;
    bool left=(pressed&BTN_L1)!=0,right=(pressed&BTN_R1)!=0;
    if(left==right)return false; /* Opposite simultaneous inputs cancel. */
    if(ui->state==UI_STATE_SETTINGS){settings_menu_input(ui,right?SYSTEM_UI_DOWN:SYSTEM_UI_UP);return true;}
    ui->state=UI_STATE_HOME;
    ui_select_tab(ui,library_tab_section((library_tab_position(ui->current_section)+(right?1:SECTION_COUNT-1))%SECTION_COUNT));
    return true;
}

void ui_navigate_left(UIContext *ui, Library *lib) {
    if (!ui || !lib) return;
    if(system_ui_input(ui,lib,SYSTEM_UI_LEFT))return;
    if(store_ui_input(ui,STORE_UI_LEFT))return;

    if(ui->state==UI_STATE_SETTINGS && ui->settings_index>=UI_LOCAL_SETTINGS_COUNT){
        const SystemFeature*feature=system_feature_get((size_t)ui->settings_index-UI_LOCAL_SETTINGS_COUNT);
        if(feature && feature->control>=0 && feature->control<=SYS_BRIGHTNESS){
            int value=feature->control==SYS_VOLUME?(ui->system_volume>=0?ui->system_volume:15):(ui->brightness_request>=0?ui->brightness_request:50);
            value+=-1*(feature->control==SYS_VOLUME?1:5);
            SystemControlResult result=system_control_set((SystemControl)feature->control,value);
            if(result.status==SYS_APPLIED)ui_observe_volume(ui,result.value,true);
            if((result.status==SYS_APPLIED_UNVERIFIED || result.status==SYS_BRIGHTNESS_SAVED))ui->brightness_request=result.value;
            ui_show_toast(ui,system_control_message(result.status));return;
        }
    }
    if(ui->state==UI_STATE_STORE){if(ui->store_index>0)ui->store_index--;return;}
    if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE){ui->store_action_index=0;return;}
    if (ui->state == UI_STATE_HOME) {
        if (ui->selected_card_index > 0) {
            ui->selected_card_index--;
        }
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        if (ui->details_button_index > 0) ui->details_button_index--;
    } else if (ui->state == UI_STATE_QUICK_MENU) {
        if (ui->quick_menu_index > 0) ui->quick_menu_index--;
    } else if (ui->state == UI_STATE_SETTINGS) {
        if (ui->settings_index > 0) ui->settings_index--;
    }
}

void ui_navigate_right(UIContext *ui, Library *lib) {
    if (!ui || !lib) return;
    if(system_ui_input(ui,lib,SYSTEM_UI_RIGHT))return;
    if(store_ui_input(ui,STORE_UI_RIGHT))return;

    if(ui->state==UI_STATE_SETTINGS && ui->settings_index>=UI_LOCAL_SETTINGS_COUNT){
        const SystemFeature*feature=system_feature_get((size_t)ui->settings_index-UI_LOCAL_SETTINGS_COUNT);
        if(feature && feature->control>=0 && feature->control<=SYS_BRIGHTNESS){
            int value=feature->control==SYS_VOLUME?(ui->system_volume>=0?ui->system_volume:15):(ui->brightness_request>=0?ui->brightness_request:50);
            value+=1*(feature->control==SYS_VOLUME?1:5);
            SystemControlResult result=system_control_set((SystemControl)feature->control,value);
            if(result.status==SYS_APPLIED)ui_observe_volume(ui,result.value,true);
            if((result.status==SYS_APPLIED_UNVERIFIED || result.status==SYS_BRIGHTNESS_SAVED))ui->brightness_request=result.value;
            ui_show_toast(ui,system_control_message(result.status));return;
        }
    }
    if(ui->state==UI_STATE_STORE){if(ui->store_index<lib->count)ui->store_index++;return;}
    if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE){if(ui_selected_app(ui,lib))ui->store_action_index=1;return;}
    if (ui->state == UI_STATE_HOME) {
        int max_count = library_section_count(lib,ui->current_section);
        if (ui->selected_card_index < max_count - 1) {
            ui->selected_card_index++;
        }
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        if (ui->details_button_index < 2) ui->details_button_index++;
    } else if (ui->state == UI_STATE_QUICK_MENU) {
        if (ui->quick_menu_index < 3) ui->quick_menu_index++;
    } else if (ui->state == UI_STATE_SETTINGS) {
        if (ui->settings_index < (int)system_feature_count()+UI_LOCAL_SETTINGS_COUNT-1) ui->settings_index++;
    }
}

void ui_navigate_up(UIContext *ui, Library *lib) {
    (void)lib;
    if (!ui) return;
    if(system_ui_input(ui,lib,SYSTEM_UI_UP))return;
    if(store_ui_input(ui,STORE_UI_UP))return;
    if (ui->state == UI_STATE_SETTINGS) {
        if (ui->settings_index > 0) ui->settings_index--;
    }
}

void ui_navigate_down(UIContext *ui, Library *lib) {
    (void)lib;
    if (!ui) return;
    if(system_ui_input(ui,lib,SYSTEM_UI_DOWN))return;
    if(store_ui_input(ui,STORE_UI_DOWN))return;
    if (ui->state == UI_STATE_SETTINGS) {
        if (ui->settings_index < (int)system_feature_count()+UI_LOCAL_SETTINGS_COUNT-1) ui->settings_index++;
    }
}

AppItem *ui_selected_app(UIContext *ui,Library *lib){
    if(!ui || !lib)return NULL;
    if(ui->current_section==SECTION_STORE){
        for(int i=0;i<lib->count;i++)if(lib->items[i].exists && !strcmp(lib->items[i].title_id,ui->store_id))return &lib->items[i];
        return NULL;
    }
    return library_get_section_app(lib,ui->current_section,ui->selected_card_index);
}
void ui_open_store_picker(UIContext *ui,Library *lib){
    ui->state=UI_STATE_STORE;ui->store_index=0;
    for(int i=0;i<lib->count;i++)if(!strcmp(lib->items[i].title_id,ui->store_id)){ui->store_index=i+1;break;}
}
void ui_observe_volume(UIContext *ui,int level,bool key_event){
    if(!ui || level<0 || level>30)return;
    if(key_event || (ui->system_volume>=0 && level!=ui->system_volume))ui->volume_timer=2.0f;
    ui->system_volume=level;
}
static void launch_selected(UIContext *ui,Library *lib,AppItem *app){
    if(app && app->media_raw && bootstrap_busy()){ui_show_toast(ui,"Componente PSP em preparacao. Tenta novamente quando terminar.");return;}
    if(store_busy()){ui_show_toast(ui,"Instalador VPK/ZIP local ativo. Aguarda; PKGs usam LiveArea.");return;}
    if(!app)return;
    if(app->media_raw){char message[256];if(!psp_create_bubble(app,message,sizeof(message))){ui_show_toast(ui,message);return;}ui->rescan_requested=true;}
    int previous_count=app->launch_count;uint64_t previous_time=app->last_played;
    library_record_launch(lib,app->title_id);
    if(!library_save_userdata(lib,VS_USERDATA_PATH)){
        app->launch_count=previous_count;app->last_played=previous_time;
        ui_show_toast(ui,"Nao foi possivel guardar o historico");return;
    }
    audio_preview_select(NULL);ui->audio_preview_pause=2.0f;
    if(launcher_launch_app(app->title_id)){ui->exit_requested=launcher_exit_required();ui_show_toast(ui,"Pedido de abertura enviado.");}
    else{app->launch_count=previous_count;app->last_played=previous_time;library_save_userdata(lib,VS_USERDATA_PATH);ui_show_toast(ui,launcher_error_message());}
}

void ui_finish_pending_launch(UIContext*ui,Library*lib){
    if(!ui->pending_launch_id[0])return;
    char id[32];snprintf(id,sizeof(id),"%s",ui->pending_launch_id);ui->pending_launch_id[0]=0;
    for(int i=0;i<lib->count;i++)if(!strcmp(lib->items[i].title_id,id)){launch_selected(ui,lib,&lib->items[i]);return;}
    ui_show_toast(ui,"A aplicacao selecionada deixou de estar disponivel.");
}

void ui_confirm(UIContext *ui, Library *lib) {
    if (!ui || !lib) return;

    if(system_ui_input(ui,lib,SYSTEM_UI_CONFIRM))return;
    if(store_ui_input(ui,STORE_UI_CONFIRM))return;
    if(ui->state==UI_STATE_SPLASH){ui->state=UI_STATE_HOME;return;}
    bool previous_sound=ui->sound_enabled,previous_animations=ui->animations_enabled;
    if(ui->state==UI_STATE_STORE){
        char previous[32];snprintf(previous,sizeof(previous),"%s",ui->store_id);
        if(ui->store_index==0)ui->store_id[0]=0;
        else if(ui->store_index<=lib->count)snprintf(ui->store_id,sizeof(ui->store_id),"%s",lib->items[ui->store_index-1].title_id);
        else{ui_show_toast(ui,"Escolhe uma aplicacao valida");return;}
        if(!settings_save(ui,VS_CONFIG_PATH)){
            snprintf(ui->store_id,sizeof(ui->store_id),"%s",previous);
            ui_show_toast(ui,"Nao foi possivel guardar a loja. Tenta novamente.");return;
        }
        ui_select_tab(ui,SECTION_STORE);ui->state=UI_STATE_HOME;
        ui_show_toast(ui,"A tua loja principal foi guardada.");return;
    }
    if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE){
        AppItem *store=ui_selected_app(ui,lib);
        if(store && ui->store_action_index==0)launch_selected(ui,lib,store);else ui_open_store_picker(ui,lib);return;
    }
    if(ui->state==UI_STATE_SETTINGS && ui->settings_index>=9 && ui->settings_index<12){
        int before_env=ui->environment_theme,before_text=ui->text_theme;bool before_contrast=ui->high_contrast;
        if(ui->settings_index==9)ui->environment_theme=(ui->environment_theme+1)%VS_THEME_COUNT;
        if(ui->settings_index==10)ui->text_theme=(ui->text_theme+1)%5;
        if(ui->settings_index==11)ui->high_contrast=!ui->high_contrast;
        if(!settings_save(ui,VS_CONFIG_PATH)){ui->environment_theme=before_env;ui->text_theme=before_text;ui->high_contrast=before_contrast;ui_show_toast(ui,"Nao foi possivel guardar as cores.");}return;
    }
    if (ui->state == UI_STATE_HOME) {
        AppItem *app = ui_selected_app(ui,lib);
        if (app) launch_selected(ui,lib,app);
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        AppItem *app = ui_selected_app(ui,lib);
        if (app) {
            if (ui->details_button_index == 0) {
                launch_selected(ui,lib,app);
            } else if(ui->details_button_index==2){
                app->type=app->type==APP_TYPE_HOMEBREW?APP_TYPE_VITA:APP_TYPE_HOMEBREW;
                app->classification_override=true;
                app->section=app->type==APP_TYPE_HOMEBREW?SECTION_HOMEBREW:SECTION_GAMES;
                library_update_indices(lib);library_save_userdata(lib,VS_USERDATA_PATH);
                ui->state=UI_STATE_HOME;ui->selected_card_index=0;
            } else if (ui->details_button_index == 1) {
                ui_toggle_favorite(ui, lib);
            }
        }
    } else if (ui->state == UI_STATE_QUICK_MENU) {
        if(ui->quick_menu_index==0)settings_menu_open(ui);
        else if(ui->quick_menu_index==1){ui->sound_enabled=!ui->sound_enabled;audio_set_sfx_enabled(ui->sound_enabled);}
        else if(ui->quick_menu_index==2){ui_select_tab(ui,SECTION_STORE);ui->state=UI_STATE_HOME;}
        else{ui->rescan_requested=true;ui->state=UI_STATE_LOADING;}
    } else if (ui->state == UI_STATE_SETTINGS) {
        if(ui->settings_index==4){
            bool requested=!ui->autoboot_enabled;
            if(autoboot_set(requested)){ui->autoboot_enabled=requested;ui_show_toast(ui,requested?"Arranque automatico ligado. Reinicia para testar.":"Arranque automatico desligado. LiveArea no proximo arranque.");}
            else ui_show_toast(ui,"Nao foi possivel guardar. Verifica o plugin de arranque.");
            return;
        }
        if(ui->settings_index>=UI_LOCAL_SETTINGS_COUNT){
            const SystemFeature*feature=system_feature_get((size_t)ui->settings_index-UI_LOCAL_SETTINGS_COUNT);
            if(feature && feature->control>=0 && feature->control<=SYS_BRIGHTNESS){
                int value=feature->control==SYS_VOLUME?(ui->system_volume>=0?ui->system_volume:15):(ui->brightness_request>=0?ui->brightness_request:50);
                SystemControlResult result=system_control_set((SystemControl)feature->control,value);
                if(result.status==SYS_APPLIED)ui_observe_volume(ui,result.value,true);
                if((result.status==SYS_APPLIED_UNVERIFIED || result.status==SYS_BRIGHTNESS_SAVED))ui->brightness_request=result.value;
                ui_show_toast(ui,system_control_message(result.status));
            }else ui_show_toast(ui,"Opção indisponível.");
            return;
        }
        if (ui->settings_index == 0) {
            ui->sound_enabled = !ui->sound_enabled;
            audio_set_sfx_enabled(ui->sound_enabled);
            ui_show_toast(ui, ui->sound_enabled ? "Efeitos sonoros ligados" : "Efeitos sonoros desligados");
        } else if (ui->settings_index == 1) {
            ui_open_store_picker(ui,lib);
        } else if (ui->settings_index == 2) {
            ui->animations_enabled = !ui->animations_enabled;
            ui_show_toast(ui, ui->animations_enabled ? "Animacoes ligadas" : "Animacoes desligadas");
        } else if (ui->settings_index == 3) {
            ui->state = UI_STATE_HOME;
        }
    }
    if((previous_sound!=ui->sound_enabled || previous_animations!=ui->animations_enabled) && !settings_save(ui,VS_CONFIG_PATH)){
        ui->sound_enabled=previous_sound;ui->animations_enabled=previous_animations;
        audio_set_sfx_enabled(previous_sound);ui_show_toast(ui,"Nao foi possivel guardar as definicoes");
    }
}

void ui_cancel(UIContext *ui) {
    if (!ui) return;
    ui->pending_launch_id[0]=0;
    if(system_ui_input(ui,NULL,SYSTEM_UI_CANCEL))return;
    if(store_ui_input(ui,STORE_UI_CANCEL))return;
    if (ui->state == UI_STATE_GAME_DETAILS || ui->state == UI_STATE_QUICK_MENU || ui->state == UI_STATE_SETTINGS || ui->state==UI_STATE_STORE) {
        ui->state = UI_STATE_HOME;
    }
}

void ui_toggle_favorite(UIContext *ui, Library *lib) {
    if(ui && ui->state==UI_STATE_REMOVE_GAME)return;
    if(ui && ui->state==UI_STATE_UNINSTALL){ui->uninstall_restore=!ui->uninstall_restore;return;}
    if (!ui || !lib) return;
    if(store_ui_input(ui,STORE_UI_REFRESH))return;
    AppItem *app = ui_selected_app(ui,lib);
    if (app) {
        library_toggle_favorite(lib, app->title_id);
        bool saved=library_save_userdata(lib, VS_USERDATA_PATH);
        ui_show_toast(ui, saved?(app->is_favorite?"Adicionado aos favoritos":"Removido dos favoritos"):"Favorito alterado, mas nao foi possivel guardar");
    }
}

void ui_open_options(UIContext *ui) {
    if (!ui) return;
    if(ui->state==UI_STATE_PLUGIN_CATALOG){store_autoplugin_request(-1);return;}
    if(store_ui_input(ui,STORE_UI_INFO))return;
    if (ui->state == UI_STATE_HOME) {
        ui->state=UI_STATE_GAME_SUPPORT;ui->game_option=0;ui->game_result=0;
    }
}

void ui_open_quick_menu(UIContext *ui) {
    if (!ui) return;
    if(ui->state==UI_STATE_SYSTEM_APPS || ui->state==UI_STATE_PLUGIN_CATALOG || ui->state==UI_STATE_REMOVE_GAME || ui->state==UI_STATE_GAME_SUPPORT || ui->state==UI_STATE_UNINSTALL || ui->state==UI_STATE_POWER || ui->state==UI_STATE_PLUGINS)return;
    if (ui->state == UI_STATE_QUICK_MENU) {
        ui->state = UI_STATE_HOME;
    } else {
        ui->state = UI_STATE_QUICK_MENU;
        ui->quick_menu_index = 0;
    }
}

void ui_handle_touch(UIContext *ui, Library *lib, int tx, int ty) {
    if (!ui || !lib) return;
    if(system_ui_touch(ui,lib,tx,ty))return;
    if(store_ui_touch(ui,tx,ty))return;

    if(ui->state==UI_STATE_STORE){
        if(ty>140 && ty<380) { if(tx<340)ui_navigate_left(ui,lib);else if(tx>620)ui_navigate_right(ui,lib);else ui_confirm(ui,lib); } return;
    }
    if (ty < 60 && ui->state==UI_STATE_HOME) {
        if(tx>=0 && tx<32){ui_handle_category_buttons(ui,BTN_L1);return;}
        if(tx>=648 && tx<680){ui_handle_category_buttons(ui,BTN_R1);return;}
        if(tx>=690 && tx<724){ui_open_quick_menu(ui);return;}
        if(tx>=734 && tx<774){settings_menu_open(ui);return;}
        if(tx>=40 && tx<40+SECTION_COUNT*UI_TAB_WIDTH) ui_select_tab(ui,library_tab_section((tx-40)/UI_TAB_WIDTH));
        return;
    }
    if(ui->state==UI_STATE_HOME && ty>=410 && ty<=460 && tx>=60 && tx<280){ui->store_action_index=0;ui_confirm(ui,lib);if(ui->state==UI_STATE_GAME_DETAILS)ui_confirm(ui,lib);return;}

    if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE && ty>=410 && ty<=460 && tx>=300 && tx<550){ui_open_store_picker(ui,lib);return;}
    if(ui->state==UI_STATE_HOME && ty>=410 && ty<=460 && ui->current_section!=SECTION_STORE){
        if(tx>=300 && tx<344){ui_toggle_favorite(ui,lib);return;}
        if(tx>=360 && tx<404){ui->state=UI_STATE_GAME_DETAILS;ui->details_button_index=0;return;}
    }
    if (ui->state == UI_STATE_HOME) {
        int count = library_section_count(lib,ui->current_section);
        int base_x = 60;

        for (int i = 0; i < count; i++) {
            int card_x = base_x + (int)carousel_offset(ui,i) - (int)ui->scroll_offset;
            int card_w = (int)(CARD_BASE_WIDTH*ui->card_scales[i]);
            int card_h = (int)(CARD_BASE_HEIGHT*ui->card_scales[i]);
            int card_y = CAROUSEL_Y - (i == ui->selected_card_index ? 12 : 0);

            if (tx >= card_x && tx <= card_x + card_w && ty >= card_y && ty <= card_y + card_h) {
                if (ui->selected_card_index == i) {
                    ui_confirm(ui, lib);
                } else {
                    ui->selected_card_index = i;
                }
                break;
            }
        }
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        if (ty >= 240 && ty <= 290) {
            if (tx >= 80 && tx <= 260) {
                ui->details_button_index = 0;
                ui_confirm(ui, lib);
            } else if (tx >= 280 && tx <= 460) {
                ui->details_button_index = 1;
                ui_confirm(ui, lib);
            } else if(tx>=480 && tx<=760){ui->details_button_index=2;ui_confirm(ui,lib);}
        }
    } else if (ui->state == UI_STATE_QUICK_MENU) {
        int panel_y = SCREEN_HEIGHT - 168;
        if (ty >= panel_y + 62 && ty <= panel_y + 111) {
            for (int i = 0; i < 4; i++) {
                int item_x = 60 + i * 215;
                if (tx >= item_x - 10 && tx <= item_x + 205) {
                    ui->quick_menu_index = i;
                    ui_confirm(ui, lib);
                    break;
                }
            }
        }
    } else if (ui->state == UI_STATE_SETTINGS) {
        int first=ui->settings_index>5?ui->settings_index-5:0;
        if(tx>=36 && tx<407 && ty>=122 && ty<472){int index=first+(ty-122)/50;
            if(index<(int)system_feature_count()+UI_LOCAL_SETTINGS_COUNT)ui->settings_index=index;
        }else if(tx>=455 && tx<900 && ty>=385 && ty<450)ui_confirm(ui,lib);
    }
}

void ui_update(UIContext *ui, Library *lib, TextureCache *cache, float delta_time) {
    if(ui)ui->updater_activity=fmodf(ui->updater_activity+delta_time,1000.0f);
    if(ui && ui->audio_preview_pause>0)ui->audio_preview_pause=fmaxf(0,ui->audio_preview_pause-delta_time);
    if (!ui || !lib) return;
    if(!isfinite(delta_time) || delta_time<0)delta_time=0;
    if(delta_time>0.1f)delta_time=0.1f;
    system_ui_update(ui,delta_time);
    if(ui->state==UI_STATE_SPLASH){ui->splash_time+=delta_time;if(ui->splash_time>=1.8f)ui->state=UI_STATE_HOME;return;}
    if(ui->volume_timer>0)ui->volume_timer=fmaxf(0,ui->volume_timer-delta_time);
#if defined(__vita__)
    ui->volume_poll-=delta_time;
    if(ui->volume_poll<=0){
        int level=-1;ui->volume_poll=.08f;int result=sceAVConfigGetSystemVol(&level);
        if(result>=0){ui_observe_volume(ui,level,false);ui->volume_error_logged=false;}
        else if(!ui->volume_error_logged){vs_log("volume","system volume read failed",result);ui->volume_error_logged=true;}
    }
#endif
#if defined(__vita__)
    static float brightness_poll=0;brightness_poll-=delta_time;
    if(ui->state==UI_STATE_SETTINGS && brightness_poll<=0){
        brightness_poll=.5f;SystemControlResult read=system_control_read(SYS_BRIGHTNESS);ui->brightness_request=read.status==SYS_BRIGHTNESS_SAVED?read.value:-1;
    }
#endif
    int count=library_section_count(lib,ui->current_section);
    if(ui->selected_card_index>=count)ui->selected_card_index=count?count-1:0;
    if(lib->scan_complete && ui->store_id[0]){bool found=false;for(int i=0;i<lib->count;i++)if(!strcmp(lib->items[i].title_id,ui->store_id))found=true;
      if(!found){ui->store_id[0]=0;settings_save(ui,VS_CONFIG_PATH);ui_show_toast(ui,"A loja selecionada ja nao esta instalada");}}
#if defined(__vita__)
    SceDateTime now;
    if (sceRtcGetCurrentClockLocalTime(&now) >= 0) {
        snprintf(ui->clock_str, sizeof(ui->clock_str), "%02d:%02d", (int)(now.hour % 24), (int)(now.minute % 60));
    }
#else
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (timeinfo) {
        snprintf(ui->clock_str, sizeof(ui->clock_str), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    }
#endif

#if defined(__vita__)
    int bat = scePowerGetBatteryLifePercent();
    ui->battery_percent = (bat >= 0 && bat <= 100) ? bat : 100;
#else
    ui->battery_percent = -1;
#endif

    if (ui->toast_timer > 0.0f) {
        ui->toast_timer -= delta_time;
    }

    for(int i=0;i<MAX_APPS;i++){float target=i==ui->selected_card_index?1.6875f:1;ui->card_scales[i]=ui->animations_enabled?ui->card_scales[i]+(target-ui->card_scales[i])*(1-expf(-12*delta_time)):target;}
    ui->target_scroll_offset = carousel_offset(ui,ui->selected_card_index);

    float lerp_speed = ui->animations_enabled ? 12.0f : 30.0f;
    ui->scroll_offset = ui->animations_enabled ? ui->scroll_offset+(ui->target_scroll_offset-ui->scroll_offset)*(1-expf(-lerp_speed*delta_time)):ui->target_scroll_offset;

    AppItem *curr_app = ui_selected_app(ui,lib);
    Texture *wanted = NULL;
    if (curr_app && curr_app->pic_path[0]) {
        wanted = texture_cache_load(cache, curr_app->pic_path);
#if defined(__vita__)
        if (wanted && !wanted->vita_tex) wanted = NULL;   /* placeholder: nao ha arte para mostrar */
#endif
    }

    if(curr_app && !wanted){
        char path[256];snprintf(path,sizeof(path),"ux0:data/VitaStation/backgrounds/%s.png",curr_app->title_id);FILE*f=fopen(path,"rb");if(f){fclose(f);wanted=texture_cache_load(cache,path);}
        if(!wanted && curr_app->icon_path[0])wanted=texture_cache_load(cache,curr_app->icon_path);
        if(!wanted){const char*suffixes[]={".png",".cover.jpg",".jpg"};for(unsigned i=0;i<3 && !wanted;i++){char name[96],key[128];snprintf(name,sizeof(name),"%s%s",curr_app->title_id,suffixes[i]);if(cover_pack_find(name,key,sizeof(key)))wanted=texture_cache_load(cache,key);}}
        if(!curr_app->pic_path[0])store_request_background(curr_app->title_id);
    }

    if (wanted != ui->target_bg) {          /* selecao mudou: recomecar a transicao */
        if(ui->target_bg)ui->target_bg->pins--;
        ui->target_bg = wanted;
        if(wanted)wanted->pins++;
        ui->bg_crossfade_alpha = ui->animations_enabled ? 0.0f : 1.0f;
    }
    if (ui->current_bg != ui->target_bg) {
        ui->bg_crossfade_alpha += delta_time * 3.0f;
        if (ui->bg_crossfade_alpha >= 1.0f) {
            if(ui->current_bg)ui->current_bg->pins--;
            ui->current_bg = ui->target_bg;
            if(ui->current_bg)ui->current_bg->pins++;
            ui->bg_crossfade_alpha = 1.0f;
        }
    } else {
        ui->bg_crossfade_alpha = 1.0f;
    }
}

static void draw_ps5_top_bar(UIContext *ui) {
    const char *tabs[]={"Jogos","Vita","Homebrew","Recentes","Favoritos","Loja","PSP / PS1"};
    render_fill_rounded_rect(8,18,23,22,7,0x38374353);render_draw_text(14,34,COLOR_WHITE,.6f,"L");
    for(int i=0;i<SECTION_COUNT;i++){
        bool selected=ui->current_section==library_tab_section(i);
        float tab_scale=.68f;while(tab_scale>.45f && render_text_width(tab_scale,tabs[i])>UI_TAB_WIDTH-7)tab_scale-=.025f;
        render_draw_text(40+i*UI_TAB_WIDTH,37,selected?COLOR_WHITE:0x94DDE4ED,tab_scale,tabs[i]);
        if(selected)render_fill_rounded_rect(40+i*UI_TAB_WIDTH,48,30,2,1,0xEAF2F6FF);
    }
    render_fill_rounded_rect(650,18,23,22,7,0x38374353);render_draw_text(656,34,COLOR_WHITE,.6f,"R");
    draw_more_symbol(699,28,0xEBF1F5FC);
    draw_settings_symbol(744,23,0xEBF1F5FC);
    render_draw_text(788,37,COLOR_WHITE,.85f,ui->clock_str[0]?ui->clock_str:"--:--");
    char battery[16];if(ui->battery_percent<0)snprintf(battery,sizeof(battery),"--");else snprintf(battery,sizeof(battery),"%d%%",ui->battery_percent);
    render_draw_text(881,37,0xCDE8EDF4,.75f,battery);
}

static void draw_ps5_bottom_bar(UIContext *ui) {
    render_fill_rect(0, SCREEN_HEIGHT - 35, SCREEN_WIDTH, 35, 0x40090C10);


    if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE){
        render_draw_text(40,SCREEN_HEIGHT-16,COLOR_WHITE,.7f,"X Confirmar    Esquerda/Direita Escolher acao    O Voltar    L/R Categorias");
    } else if (ui->state == UI_STATE_HOME) {
        render_draw_text(40, SCREEN_HEIGHT - 16, COLOR_WHITE, 0.65f, "X Abrir    O Voltar    Quadrado Favorito    Triangulo Opcoes do jogo    START Centro de controlo");
    } else if(ui->state==UI_STATE_SETTINGS){
        render_draw_text(40,SCREEN_HEIGHT-16,COLOR_WHITE,.75f,ui->settings_group==SETTINGS_ROOT?"X Abrir    O Voltar":"X Selecionar    O Categorias    Setas Navegar");
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        render_draw_text(40, SCREEN_HEIGHT - 16, COLOR_WHITE, 0.8f, "X Confirmar    O Voltar    Quadrado Favorito");
    } else {
        render_draw_text(40, SCREEN_HEIGHT - 16, COLOR_WHITE, 0.8f, "X Confirmar    O Fechar");
    }
}

static void draw_toast_notification(UIContext *ui) {
    if (ui->toast_timer > 0.0f && ui->toast_msg[0] != '\0') {
        int w = 360;
        int h = 40;
        int x = (SCREEN_WIDTH - w) / 2;
        int y = 470;
        render_fill_rect(x, y, w, h, COLOR_PANEL_BG);
        render_draw_rect(x,y,w,h,0x50778598,1);
        render_draw_text(x + 20, y + 26, COLOR_WHITE, 0.9f, ui->toast_msg);
    }
}

static void draw_store_home(UIContext *ui,Library *lib,TextureCache *cache){
    AppItem *store=ui_selected_app(ui,lib);
    render_draw_text(60,104,0xBCD6DFEB,.75f,"VITASTATION STORE");
    if(store){
        Texture *icon=texture_cache_load(cache,store->icon_path);
        render_fill_rounded_rect(60,124,104,104,16,0x95233043);
        if(icon)render_draw_texture(icon,64,128,96,96,1);
        draw_title(190,162,1.25f,store->title,26);
        render_draw_text(190,194,COLOR_TEXT_MUTED,.75f,"A tua loja principal");
        int shown=0;
        for(int i=0;i<lib->count && shown<3;i++){
            AppItem *entry=&lib->items[i];if(entry==store || !entry->exists)continue;
            if(!shown)render_draw_text(580,104,COLOR_TEXT_MUTED,.7f,"Da tua biblioteca");
            int x=580+shown*98;
            render_fill_rounded_rect(x,128,84,84,12,0xA5253040);
            Texture *art=texture_cache_load(cache,entry->icon_path);
            if(art)render_draw_texture(art,x+3,131,78,78,.85f);
            char caption[16];snprintf(caption,sizeof(caption),"%.10s",entry->title);
            render_draw_text(x,234,COLOR_TEXT_MUTED,.55f,caption);shown++;
        }
        draw_title(60,343,1.95f,"O teu proximo jogo comeca aqui.",44);
        render_draw_text(60,378,COLOR_TEXT_MUTED,.85f,"Abre a aplicacao de loja que escolheste.");
    }else{
        draw_title(60,310,1.95f,"A tua loja. A tua escolha.",44);
        render_draw_text(60,350,COLOR_TEXT_MUTED,.8f,"Escolhe uma aplicacao instalada como loja principal.");
    }
    render_fill_rounded_rect(60,410,220,48,24,ui->store_action_index==0?0xF5F1F3F8:0x80252D39);
    render_draw_text(100,441,ui->store_action_index==0?0xFF151923:COLOR_WHITE,1.0f,store?"Abrir loja":"Escolher loja");
    if(store){render_fill_rounded_rect(300,412,210,44,22,ui->store_action_index==1?0xF5F1F3F8:0x80252D39);render_draw_text(330,440,ui->store_action_index==1?0xFF151923:COLOR_WHITE,.85f,"Alterar loja");}
}
static void draw_volume_hud(UIContext *ui){
    if(ui->volume_timer<=0 || ui->system_volume<0)return;
    int x=340,y=58;
    render_fill_rounded_rect(x,y,280,44,18,0xF01B222D);
    render_draw_text(x+16,y+28,COLOR_WHITE,.75f,"Volume");
    render_fill_rounded_rect(x+83,y+19,140,5,2,0xFF414956);
    int width=140*ui->system_volume/30;if(width>0)render_fill_rounded_rect(x+83,y+19,width,5,2,0xFFF2F5FC);
    char number[12];snprintf(number,sizeof(number),"%d%%",ui->system_volume*100/30);
    render_draw_text(x+235,y+28,COLOR_WHITE,.7f,number);
}

static void draw_splash(UIContext *ui,TextureCache *cache){
    float fade=ui->splash_time<=1.3f?1:fmaxf(0,1-(ui->splash_time-1.3f)/.5f);
    uint32_t white=((uint32_t)(255*fade)<<24)|0xF2F5FC;
    for(int y=0;y<544;y+=8){float glow=expf(-powf((y-260)/240.0f,2));unsigned c=(unsigned)(10+18*glow);render_fill_rect(0,y,960,8,0xFF000000|(c/2<<16)|(c<<8)|(c+14));}
    Texture*logo=texture_cache_load(cache,"app0:assets/brand-logo.png");
    if(logo)render_draw_texture(logo,424,134,112,112,fade);
    const char *title="Vita Station",*credit="desenvolvido por Carlos Tuga";
    render_draw_text((960-render_text_width(2.3f,title))/2,302,white,2.3f,title);
    render_draw_text((960-render_text_width(.85f,credit))/2,340,((uint32_t)(170*fade)<<24)|0xDDE6F3,.85f,credit);
    render_fill_rounded_rect(455,377,50,2,1,((uint32_t)(125*fade)<<24)|0xB2D7FF);
}

void ui_render(UIContext *ui, Library *lib, TextureCache *cache) {
    if(ui)theme_configure(ui->environment_theme,ui->text_theme,ui->high_contrast);
    if (!ui || !lib) return;

    render_begin_frame();
    render_clear(COLOR_BG_DARK);

    render_fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG_DARK);
    if(ui->state==UI_STATE_SPLASH){draw_splash(ui,cache);render_end_frame();return;}
    if(ui->wallpaper_path[0] || (!ui->current_bg && !ui->target_bg))personalization_background(ui,cache);
    if (!ui->wallpaper_path[0] && ui->current_bg) {
        /* sem fundo novo: o atual desvanece; com fundo novo: fica opaco por baixo */
        float a = (ui->current_bg != ui->target_bg && !ui->target_bg) ? 1.0f - ui->bg_crossfade_alpha : 1.0f;
        render_draw_texture_cover(ui->current_bg,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,a);
    }
    if (!ui->wallpaper_path[0] && ui->target_bg && ui->target_bg != ui->current_bg) {
        render_draw_texture_cover(ui->target_bg,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,ui->bg_crossfade_alpha);
    }

    /* Procedural shade preserves artwork and text contrast. */
    for(int x=0;x<640;x+=16){unsigned a=(unsigned)(135*(1-x/640.0f));render_fill_rect(x,60,16,449,(a<<24)|0x050810);}
    for(int y=220;y<509;y+=8){unsigned a=(unsigned)(125*(y-220)/289);render_fill_rect(0,y,960,8,(a<<24)|0x04070C);}
    draw_ps5_top_bar(ui);

    if(personalization_render(ui,cache)){}
    else if(system_ui_render(ui,lib)){}
    else if(ui->state==UI_STATE_LOADING){render_draw_text(60,250,COLOR_WHITE,1.4f,"A descobrir aplicacoes...");}
    else if(ui->catalog && (ui->state==UI_STATE_CATALOG_DETAILS || (ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE))){store_ui_render(ui,cache);}
    else if(ui->state==UI_STATE_HOME && ui->current_section==SECTION_STORE){draw_store_home(ui,lib,cache);}
    else if (ui->state == UI_STATE_HOME) {
        AppItem *curr_app = ui_selected_app(ui,lib);

        if (curr_app) {
            draw_title(60,348,2.05f,curr_app->title[0]?curr_app->title:curr_app->title_id,44);
            char info_line[128];
            snprintf(info_line, sizeof(info_line), "Disponivel na tua biblioteca%s",
                curr_app->is_favorite ? " | Favorito" : "");
            render_draw_text(60, 378, COLOR_TEXT_MUTED, 0.9f, info_line);
            render_draw_text(60,295,0xBCD6DFEB,.68f,curr_app->type==APP_TYPE_SYSTEM?"APLICACOES | VITA":curr_app->type==APP_TYPE_UNKNOWN?"APLICACAO INSTALADA":curr_app->type==APP_TYPE_HOMEBREW?"HOMEBREW | PS VITA":"JOGOS | PS VITA");
        } else {
            render_draw_text(60, 343, COLOR_WHITE, 1.5f, (ui->library_loading?"A carregar a biblioteca em segundo plano...":ui->current_section==SECTION_GAMES?"Nenhum jogo encontrado.":ui->current_section==SECTION_HOMEBREW?"Nenhuma aplicacao Homebrew encontrada.":ui->current_section==SECTION_VITA?"Adiciona aplicacoes em Definicoes > VitaStation > Aplicacoes da categoria Vita.":ui->current_section==SECTION_PSP_PS1?"Nenhum jogo PSP ou PS1 encontrado.":ui->current_section==SECTION_RECENT?"Ainda nao existem aplicacoes recentes.":"Ainda nao existem favoritos."));
            render_draw_text(60, 378, COLOR_TEXT_MUTED, 0.9f, "START: menu | Reconstruir base de dados nas definicoes");
        }

        if(curr_app){
            render_fill_rounded_rect(56,406,228,56,28,0x165BBBEA);
            render_fill_rounded_rect(60,410,220,48,24,0xF5F1F3F8);
            for(int i=0;i<11;i++)render_fill_rect(105+i,424+i/2,1,20-i,0xFF171C26);
            render_draw_text(137,441,0xFF151923,1.05f,"Jogar");
            render_fill_rounded_rect(300,412,44,44,22,0x80252D39);
            draw_star(312,423,curr_app->is_favorite?0xFFFFD992:0xE5F4F7FF);
            render_fill_rounded_rect(360,412,44,44,22,0x80252D39);
            draw_more_symbol(374,432,0xFFF4F7FF);
        }
        int section_count = library_section_count(lib,ui->current_section);
        int base_x = 60;

        for (int i = 0; i < section_count; i++) {
            AppItem *app = library_get_section_app(lib, ui->current_section, i);
            if (!app) continue;

            int card_x = base_x + (int)carousel_offset(ui,i) - (int)ui->scroll_offset;

            if (card_x + CARD_SELECTED_WIDTH < -50 || card_x > SCREEN_WIDTH + 50) {
                continue;
            }

            bool is_selected = (i == ui->selected_card_index);
            int w = (int)(CARD_BASE_WIDTH*ui->card_scales[i]);
            int h = (int)(CARD_BASE_HEIGHT*ui->card_scales[i]);
            int y = CAROUSEL_Y - (is_selected ? 12 : 0);

            if(is_selected){
                render_fill_rounded_rect(card_x-7,y-7,w+14,h+14,17,0x1CF1F5FC);
                render_fill_rounded_rect(card_x-3,y-3,w+6,h+6,13,0xF5F1F3FA);
            }
            render_fill_rounded_rect(card_x,y,w,h,10,is_selected?0xFF25364D:0xD51B2330);
            Texture *icon=app->icon_path[0]?texture_cache_load(cache,app->icon_path):NULL;
            if(icon)render_draw_texture(icon,card_x+3,y+3,w-6,h-6,is_selected?1:.72f);
            else{
                char initial[2]={app->title[0]?app->title[0]:'?',0};
                render_draw_text(card_x+w/2-10,y+h/2+12,0xCFE3EBFA,1.5f,initial);
            }
            if(is_selected){
                char label[128];snprintf(label,sizeof(label),"%.38s",app->title);
                render_draw_text(190,194,0xDDEAF0F8,.75f,label);
            }

            if (app->is_favorite) {
                render_fill_rounded_rect(card_x+w-15,y+7,7,7,3,0xFFFFD992);
            }
        }
    } else if (ui->state == UI_STATE_GAME_DETAILS) {
        AppItem *app = ui_selected_app(ui,lib);
        if (app) {
            render_fill_rounded_rect(40,80,SCREEN_WIDTH-80,360,22,0xB80B111B);
            

            float title_scale=strlen(app->title)>32?1.2f:1.7f;
            draw_title(80,140,title_scale,app->title[0]?app->title:app->title_id,48);
            if(app->icon_path[0]){Texture*t=texture_cache_load(cache,app->icon_path);if(t)render_draw_texture(t,800,100,72,72,1);}

            char meta[128];
            snprintf(meta, sizeof(meta), "ID: %s   Categoria: %s   Pedidos: %d",
                app->title_id,
                app->category_str[0] ? app->category_str : "Nao classificada",
                app->launch_count);
            render_draw_text(80, 180, COLOR_TEXT_MUTED, 1.0f, meta);

            uint32_t play_bg = ui->details_button_index==0?0xEDF1F4FA:COLOR_CARD_BG;
            render_fill_rounded_rect(80,240,180,50,25,play_bg);
            
            render_draw_text(130,272,ui->details_button_index==0?0xFF151B25:COLOR_WHITE,1.2f,"JOGAR");

            render_draw_text(490,272,ui->details_button_index==2?COLOR_PS5_LIGHT_BLUE:COLOR_WHITE,.9f,"Classificar Homebrew/Jogos");
            char last[90]="Data indisponivel";time_t stamp=(time_t)app->last_played;
            struct tm *date=localtime(&stamp);if(date)strftime(last,sizeof(last),"Ultimo pedido: %d/%m/%Y %H:%M",date);
            render_draw_text(80,340,COLOR_TEXT_MUTED,.85f,app->last_played?last:"Ultima utilizacao: indisponivel");
            uint32_t fav_bg = ui->details_button_index==1?0xEDF1F4FA:COLOR_CARD_BG;
            render_fill_rounded_rect(280,240,180,50,25,fav_bg);
            
            render_draw_text(300,272,ui->details_button_index==1?0xFF151B25:COLOR_WHITE,.85f, app->is_favorite ? "Remover favorito" : "Favorito");
        }
    } else if (ui->state == UI_STATE_QUICK_MENU) {
        int panel_h = 118;
        int panel_y = SCREEN_HEIGHT - 168;
        render_fill_rounded_rect(40,panel_y,SCREEN_WIDTH-80,panel_h,18,COLOR_PANEL_BG);
        render_draw_rect(40, panel_y, SCREEN_WIDTH - 80, panel_h, 0x508A9BAB, 1);

        render_draw_text(60, panel_y + 26, COLOR_WHITE, 1.1f, "Centro de controlo");

        const char *items[] = {"Definicoes", "Som", "Loja", "Atualizar Biblioteca"};
        for (int i = 0; i < 4; i++) {
            int item_x = 60 + i * 215;
            int item_y = panel_y + 86;
            uint32_t col = ui->quick_menu_index==i?0xFF151B25:COLOR_WHITE;
            if (ui->quick_menu_index == i) {
                render_fill_rounded_rect(item_x-5,item_y-20,205,35,17,0xEDF1F4FA);
            }
            render_draw_text(item_x, item_y, col, 0.78f, items[i]);
        }
    } else if(ui->state==UI_STATE_STORE){
        render_fill_rounded_rect(100,110,760,300,20,COLOR_PANEL_BG);
        render_draw_text(140,155,COLOR_WHITE,1.2f,"LOJA | Escolher aplicacao instalada");
        AppItem *store=ui->store_index>0?library_get_app(lib,ui->store_index-1):NULL;
        draw_title(290,230,1.0f,store?store->title:"Predefinida: sem destino configurado",42);
        render_draw_text(140,300,COLOR_PS5_LIGHT_BLUE,1.0f,"< Anterior          Guardar          Seguinte >");
        if(store){Texture*t=texture_cache_load(cache,store->icon_path);if(t)render_draw_texture(t,160,190,96,96,1);}
        render_draw_text(140,350,COLOR_TEXT_MUTED,.85f,"Esquerda / Direita: escolher    X: guardar    O: voltar");
        render_draw_text(140,380,COLOR_TEXT_MUTED,.8f,"A selecao manual nao identifica automaticamente uma loja.");
    } else if (ui->state == UI_STATE_SETTINGS) {
        if(ui->settings_group==SETTINGS_ROOT){settings_menu_root(ui);}else{
        render_fill_rect(0,60,960,444,0xFF0F1B2E);
        render_fill_rounded_rect(444,128,472,328,22,0xFF15253B);
        render_draw_text(15,102,0xFFC0D8EE,1.2f,"<");
        render_draw_text(46,103,COLOR_WHITE,1.4f,ui->settings_group==SETTINGS_CONSOLE?"Definições da consola":"Definições do VitaStation");
        render_draw_text(46,126,0xFF9DB5CE,.65f,"O: categorias  |  Cima / Baixo: escolher");
        int position=settings_group_position(ui->settings_group,ui->settings_index);
        int first=position>5?position-5:0;
        const char*local[]={"Som do VitaStation","Loja externa predefinida","Animações do launcher","Voltar ao início","Iniciar VitaStation com a consola","Gerir plugins","Comportamento do botão PS","Desligar ou reiniciar","Instalar DLCs com o jogo","Cor do ambiente","Cor das letras","Alto contraste","Versão da aplicação","Atualizações do VitaStation","Desinstalar VitaStation e carátulas","Aplicacoes da categoria Vita","Catalogo AutoPlugin II","Apollo: gestao de saves","Personalizacao: temas e imagens"};
        int total=settings_group_count(ui->settings_group);
        for(int row=first;row<first+7 && row<total;row++){
            int i=settings_group_item(ui->settings_group,row),y=164+(row-first)*46;bool sel=i==ui->settings_index;
            if(sel)render_fill_rounded_rect(36,y-27,374,42,10,0xFFF2F4F8);
            const SystemFeature*feature=i>=UI_LOCAL_SETTINGS_COUNT?system_feature_get((size_t)i-UI_LOCAL_SETTINGS_COUNT):NULL;
            const char*name=feature?feature->name:local[i];char label[96];size_t n=strlen(name);if(n>32){n=32;while(n && (((unsigned char)name[n]&0xc0)==0x80))n--;}
            memcpy(label,name,n);label[n]=0;if(strlen(name)>n)strcat(label,"...");
            settings_option_icon(49,y-20,22,i,sel?0xFF355E89:0xFF8AABC9);
            render_draw_text(84,y,sel?0xFF172133:0xFFC5CEE0,.79f,label);
        }
        settings_option_icon(465,162,42,ui->settings_index,0xFF8ACDF7);
        const SystemFeature*feature=ui->settings_index>=UI_LOCAL_SETTINGS_COUNT?system_feature_get((size_t)ui->settings_index-UI_LOCAL_SETTINGS_COUNT):NULL;
        if(feature){
            render_draw_text(520,156,0xFF9EBDDF,.72f,feature->category);
            draw_title(520,201,1.0f,feature->name,31);
            if(feature->control>=SYS_ACTION_LIBRARY){
                render_draw_text(465,270,0xFFB7C9E0,.95f,feature->control==SYS_ACTION_REFRESH?"Reconstruir a biblioteca guardada":"Função integrada no VitaStation");
                render_draw_text(465,313,0xFFB7C9E0,.75f,feature->control==SYS_ACTION_REFRESH?"Atualiza a base de aplicações e ícones.":"X: abrir a função correspondente");
                render_fill_rounded_rect(465,388,420,44,22,0xFFF2F4F8);render_draw_text(500,418,0xFF172133,.95f,"X  Abrir funcao");
            }else if(feature->control>=SYS_INFO_NETWORK){
                render_draw_text(465,270,0xFFB7C9E0,.95f,"Consultar valores da consola");
                render_draw_text(465,313,0xFFB7C9E0,.8f,"Informacao atualizada, apenas leitura.");
                render_fill_rounded_rect(465,388,420,44,22,0xFFF2F4F8);render_draw_text(500,418,0xFF172133,.95f,"X  Consultar");
            }else if(feature->control>=0){
                char value[64];if(feature->control==SYS_VOLUME)snprintf(value,sizeof(value),"Volume: %d / 30",ui->system_volume);
                else if(ui->brightness_request>=0)snprintf(value,sizeof(value),"Brilho: %d%%",ui->brightness_request);else snprintf(value,sizeof(value),"Brilho: escolhe um valor");
                render_draw_text(465,264,COLOR_WHITE,1.1f,value);
                render_draw_text(465,308,0xFFB7C9E0,.8f,"Esquerda / Direita: alterar diretamente");
                render_draw_text(465,340,0xFFB7C9E0,.75f,feature->control==SYS_VOLUME?"O valor e lido novamente apos a alteracao.":"X: aplicar 50% se ainda nao escolheste.");
                render_fill_rounded_rect(465,388,420,44,22,0xFFF2F4F8);render_draw_text(500,418,0xFF172133,.95f,"X  Aplicar na consola");
            }else{
                render_draw_text(465,270,0xFFB7C9E0,.95f,"Opção indisponível");
                draw_title(465,313,.75f,system_feature_reason(feature),52);
                render_draw_text(465,347,0xFFB7C9E0,.8f,"Nao abre o painel original do sistema.");
            }
        }else{
            if(ui->settings_index==18){
                render_draw_text(465,220,COLOR_WHITE,1.0f,"Personalizacao");
                draw_title(465,270,.8f,"10 temas novos, fontes, wallpaper e bloqueio",45);
                render_draw_text(465,330,COLOR_WHITE,.72f,"X: escolher o ambiente e imagens");
            }else if(ui->settings_index>=12){
                UpdateStatus status;store_update_status(&status);IntegrationStatus provider;store_integration_status(&provider);
                render_draw_text(465,220,COLOR_WHITE,1.0f,VS_VERSION_LABEL);
                draw_title(465,270,.8f,ui->settings_index==12?"Desenvolvido por Carlos Tuga":ui->settings_index==14?"Remove o launcher e todas as caratulas":ui->settings_index==15?"Escolher aplicacoes adicionais para a categoria Vita":ui->settings_index==16?"Procurar e obter plugins sem abrir AutoPlugin II":ui->settings_index==17?(provider.message[0]?provider.message:"Dados de patches e gestao avancada de saves"):status.message,48);
                if(ui->settings_index==13 && status.phase==1){
                    render_fill_rounded_rect(465,292,420,12,6,0xFF30445E);
                    if(status.total){double fraction=(double)status.done/(double)status.total;if(fraction>1)fraction=1;render_fill_rounded_rect(465,292,(float)(420*fraction),12,6,0xFF67B9F5);}
                    else render_fill_rounded_rect(465+(int)(ui->updater_activity*90)%340,292,80,12,6,0xFF67B9F5);
                }
                if(ui->settings_index==13 && status.phase==3){render_draw_text(465,410,COLOR_WHITE,.65f,"A abrir o instalador automatico...");}
                render_draw_text(465,322,0xFFB7C9E0,.65f,"GitHub: CarlosTuga/VitaStation (inclui betas)");
                render_draw_text(465,370,COLOR_WHITE,.7f,ui->settings_index==14?"X: abrir confirmacao de desinstalacao":ui->settings_index>=15?"X: abrir a integracao":"X: procurar, descarregar e instalar automaticamente");
            }else{
            render_draw_text(520,165,0xFF9EBDDF,.75f,"VitaStation");
            draw_title(520,215,.95f,local[ui->settings_index],31);
            const char*value=ui->settings_index==9?theme_environment_name(ui->environment_theme):ui->settings_index==10?theme_text_name(ui->text_theme):ui->settings_index==11?(ui->high_contrast?"Ativado: texto opaco com sombra":"Contraste normal"):ui->settings_index==8?(ui->auto_dlcs_enabled?"Ligado: DLCs da mesma ID do jogo":"Desligado: instalar apenas o jogo"):ui->settings_index==5?"Ativar ou desativar plugins individuais":ui->settings_index==6?(ui->ps_routing_enabled?"Um toque: VitaStation | Dois: LiveArea":"Comportamento original do sistema"):ui->settings_index==7?"Desligar, reiniciar ou cancelar":ui->settings_index==4?(ui->autoboot_enabled?"Ligado: iniciar VitaStation":"Desligado: iniciar LiveArea"):ui->settings_index==0?(ui->sound_enabled?"Efeitos sonoros ligados":"Efeitos sonoros desligados"):ui->settings_index==2?(ui->animations_enabled?"Animacoes ligadas":"Animacoes desligadas"):ui->settings_index==1?"Escolher uma aplicacao instalada":"Regressar a biblioteca";
            draw_title(465,290,.82f,value,46);
            const char*descriptions[]={"Ativa ou silencia os efeitos do launcher.","Escolhe a loja externa usada por predefinição.","Ativa as transições visuais da interface.","Regressa à biblioteca de aplicações.","Abre VitaStation ao iniciar a consola.","Consulta e ativa plugins individualmente.","Configura um toque e duplo toque no botão PS.","Abre as opções de energia da consola.","Adiciona os DLCs disponíveis da mesma ID.","Personaliza os fundos e os painéis.","Escolhe o tom das letras da interface.","Reforça a opacidade e a sombra das letras."};
            draw_title(465,332,.69f,descriptions[ui->settings_index],55);
            render_fill_rounded_rect(465,388,420,44,22,0xFFF2F4F8);render_draw_text(500,418,0xFF172133,.95f,"X  Selecionar");
        }
        }
        }
    }

    store_ui_progress();
    draw_ps5_bottom_bar(ui);
    draw_toast_notification(ui);
    draw_volume_hud(ui);

    render_end_frame();
}
