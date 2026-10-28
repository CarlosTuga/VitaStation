#include "library.h"
#include "library_cache.h"
#include "installation_watch.h"
#include "bootstrap.h"
#include "log.h"
#include "texture.h"
#include "render.h"
#include "ui.h"
#include "input.h"
#include "audio.h"
#include "launcher.h"
#include "settings.h"
#include "store.h"
#include "plugins.h"
#include "autoboot.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#if defined(__vita__)
#include <psp2/kernel/processmgr.h>
#endif

/* Estruturas grandes (Library ~186 KB) em memoria estatica: a pilha da thread principal da Vita
 * tem tipicamente 256 KB. */
static Library lib;
static TextureCache cache;
static UIContext ui;
static StoreCatalog catalog;
static unsigned store_revision=0;
static unsigned covers_revision=0,homebrew_revision=0;
static bool homebrew_reload_pending=false;

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
#if defined(__vita__)
    uint64_t boot_start=sceKernelGetProcessTimeWide();
#endif

#if defined(__vita__)
    settings_ensure_dir("ux0:data");
#endif
    settings_ensure_dir(VS_DATA_DIR);
    vs_log("build","VitaStation 1.6.1 beta (01.61), permanent offline cover library",0);
    if(!render_init())return 1;
    input_init();

    library_init(&lib);
    bool cached=library_cache_load(&lib,VS_DATA_DIR "/library-cache.bin");
    bool rebuild=library_cache_rebuild_due(cached,VS_DATA_DIR "/library-starts.txt") || installation_dirty() || installation_cache_changed(VS_DATA_DIR "/installation-state.txt");
    library_load_userdata(&lib,VS_USERDATA_PATH);
    texture_cache_init(&cache, 32);
    ui_init(&ui);
    catalog_init(&catalog);ui.catalog=&catalog;store_start();
    settings_load(&ui, VS_CONFIG_PATH);
    ui.autoboot_enabled=autoboot_is_enabled();
    audio_set_sfx_enabled(ui.sound_enabled);
    ui.toast_timer=0;if(cached)ui.splash_time=1.4f;ui.state=UI_STATE_SPLASH;ui_render(&ui,&lib,&cache);
#if defined(__vita__)
    char first_frame[96];snprintf(first_frame,sizeof(first_frame),"first_frame_ms=%llu",(unsigned long long)((sceKernelGetProcessTimeWide()-boot_start)/1000));vs_log("startup",first_frame,0);
#endif
    audio_init();audio_set_sfx_enabled(ui.sound_enabled);
    ui.library_loading=rebuild && bootstrap_start(&lib);
    if(rebuild && !ui.library_loading)ui_show_toast(&ui,"Usa Reconstruir base de dados nas definicoes.");
    if(!rebuild)vs_log("startup","cached database: no background scan or plugin preparation",lib.count);
    bool running = true;
#if defined(__vita__)
    uint64_t previous=sceKernelGetProcessTimeWide();
#endif
    InputState input;
    unsigned completed_seen=0;float install_poll=0;uint64_t pending_signature=0;unsigned stable_polls=0;

    while (running) {
        int scanned=-1;unsigned startup=bootstrap_poll(&lib,&scanned,ui.plugins_message,sizeof(ui.plugins_message));
        if(startup&1){if(scanned>=0){library_cache_reset_counter(VS_DATA_DIR "/library-starts.txt");installation_cache_commit(VS_DATA_DIR "/installation-state.txt");installation_clear_dirty();}ui.library_loading=false;texture_cache_retry(&cache);ui_show_toast(&ui,scanned<0?"Nao foi possivel atualizar a biblioteca.":"Biblioteca atualizada.");}
        if(startup&2){
            bootstrap_shutdown();
            if(strstr(ui.plugins_message,"Reinicia")){ui_show_toast(&ui,ui.plugins_message);ui.toast_timer=8;}
            ui.autoboot_enabled=autoboot_is_enabled();
            if(autoboot_bridge_available()){
                FILE*configured=fopen(VS_DATA_DIR "/default-launcher-160.done","rb");
                if(configured)fclose(configured);else if(autoboot_set(true)){
                    ui.autoboot_enabled=true;ui.ps_routing_enabled=true;settings_save(&ui,VS_CONFIG_PATH);
                    configured=fopen(VS_DATA_DIR "/default-launcher-160.done","wb");if(configured){fputs("configured\n",configured);fclose(configured);}
                }
                autoboot_marker_write(VS_DATA_DIR "/ps-routing.enabled",ui.ps_routing_enabled);autoboot_marker_write(VS_DATA_DIR "/power-menu.enabled",true);}
            ui_finish_pending_launch(&ui,&lib);
        }
        if(ui.exit_requested){
#if defined(__vita__)
            bootstrap_shutdown();settings_save(&ui,VS_CONFIG_PATH);sceKernelExitProcess(0);
#endif
            running=false;break;
        }
        input_poll(&input);
        if(input.buttons_pressed & BTN_POWER)ui_open_power_menu(&ui);
#if defined(__vita__)
        static unsigned bridge_poll=0;
        if(++bridge_poll%15==0){
            FILE*request=fopen(VS_DATA_DIR "/power.request","rb");
            if(request){int command=fgetc(request);fclose(request);remove(VS_DATA_DIR "/power.request");if(command=='1')ui_open_power_menu(&ui);}
        }
#endif
        if(input.buttons_pressed&(BTN_VOLUME_UP|BTN_VOLUME_DOWN))ui_observe_volume(&ui,ui.system_volume,true);

        if (input.buttons_pressed & BTN_LEFT) {
            ui_navigate_left(&ui, &lib);
            audio_play_sfx(SND_MOVE);
        }
        if (input.buttons_pressed & BTN_RIGHT) {
            ui_navigate_right(&ui, &lib);
            audio_play_sfx(SND_MOVE);
        }
        if (input.buttons_pressed & BTN_UP) {
            ui_navigate_up(&ui, &lib);
            audio_play_sfx(SND_MOVE);
        }
        if (input.buttons_pressed & BTN_DOWN) {
            ui_navigate_down(&ui, &lib);
            audio_play_sfx(SND_MOVE);
        }
        if(ui_handle_category_buttons(&ui,input.buttons_pressed))audio_play_sfx(SND_SELECT);
        if (ui.state == UI_STATE_HOME) {
            for (int i = 0; i < input.swipe_steps; i++) ui_navigate_right(&ui, &lib);
            for (int i = 0; i > input.swipe_steps; i--) ui_navigate_left(&ui, &lib);
            if (input.swipe_steps != 0) audio_play_sfx(SND_MOVE);
        }
        if (input.buttons_pressed & BTN_CROSS) {
            ui_confirm(&ui, &lib);
            audio_play_sfx(SND_CONFIRM);
        }
        if (input.buttons_pressed & BTN_CIRCLE) {
            if (ui.state == UI_STATE_HOME) {
                if(store_busy())ui_show_toast(&ui,"Uma instalacao esta em curso. Aguarda a conclusao.");
                else running = false;
            } else {
                ui_cancel(&ui);
                audio_play_sfx(SND_CANCEL);
            }
        }
        if (input.buttons_pressed & BTN_SQUARE) {
            ui_toggle_favorite(&ui, &lib);
            audio_play_sfx(SND_SELECT);
        }
        if (input.buttons_pressed & BTN_TRIANGLE) {
            ui_open_options(&ui);
            audio_play_sfx(SND_SELECT);
        }
        if(input.buttons_pressed & BTN_SELECT){store_cancel();ui_show_toast(&ui,"Cancelamento pedido. Uma instalacao iniciada deve terminar.");}
        if (input.buttons_pressed & BTN_START) {
            ui_open_quick_menu(&ui);
            audio_play_sfx(SND_MENU_OPEN);
        }

        if (input.touch_pressed) {
            ui_handle_touch(&ui, &lib, input.touch_x, input.touch_y);
        }

#if defined(__vita__)
        uint64_t now=sceKernelGetProcessTimeWide();
        float dt=(float)(now-previous)/1000000.0f;previous=now;
#else
        float dt=0; running=false; /* host target is a smoke test, not a fake launcher */
#endif
        ui_update(&ui, &lib, &cache, dt);
        ui_render(&ui, &lib, &cache);
        if(ui.catalog_load_requested){
            if(ui.catalog_type==CATALOG_DEMOS)ui.catalog_type=CATALOG_GAMES;
            if(ui.catalog_type==CATALOG_HOMEBREW)store_refresh_homebrew(false);
            catalog_load_auto(&catalog,ui.catalog_type);ui.catalog_load_requested=false;ui.catalog_index=0;
        }
        StoreProgress progress;store_progress(&progress);
        if(progress.homebrew_revision!=homebrew_revision){homebrew_revision=progress.homebrew_revision;homebrew_reload_pending=true;}
        if(homebrew_reload_pending && ui.catalog_type==CATALOG_HOMEBREW && ui.state!=UI_STATE_CATALOG_DETAILS){ui.catalog_load_requested=true;homebrew_reload_pending=false;}
        if(progress.covers_revision!=covers_revision){
            covers_revision=progress.covers_revision;
            catalog_invalidate_covers(&catalog);
            texture_cache_retry(&cache);
        }
        if(progress.completed!=completed_seen){completed_seen=progress.completed;ui.rescan_requested=true;}
        install_poll+=dt;
        if(install_poll>=2.0f){install_poll=0;uint64_t signature=installation_signature();
            if(signature==pending_signature)stable_polls++;else{pending_signature=signature;stable_polls=0;}
            if(stable_polls>=2 && !store_busy() && !bootstrap_busy() && (installation_dirty() || installation_cache_changed(VS_DATA_DIR "/installation-state.txt")))ui.rescan_requested=true;
        }
        if(progress.revision!=store_revision){
            store_revision=progress.revision;ui_show_toast(&ui,progress.message);
            if(progress.phase==STORE_COMPLETE)ui.rescan_requested=true;
        }
        if(ui.rescan_requested && !bootstrap_busy()){
            texture_cache_retry(&cache);ui.rescan_requested=false;ui.state=UI_STATE_HOME;
            ui.library_loading=bootstrap_start(&lib);
            ui_show_toast(&ui,ui.library_loading?"A atualizar a biblioteca em segundo plano...":"Nao foi possivel iniciar a atualizacao.");
        }
    }

    settings_save(&ui, VS_CONFIG_PATH);
    if(lib.scan_complete || lib.count>0)library_save_userdata(&lib, VS_USERDATA_PATH);
    bootstrap_shutdown();
    store_shutdown();catalog_clear(&catalog);
    texture_cache_clear(&cache);
    audio_shutdown();
    render_shutdown();

#if defined(__vita__)
    sceKernelExitProcess(0);
#endif

    return 0;
}
