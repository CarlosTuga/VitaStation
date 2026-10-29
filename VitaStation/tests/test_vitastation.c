#include "library.h"
#include "texture.h"
#include "ui.h"
#include "settings_menu.h"
#include "settings.h"
#include "launcher.h"
#include "audio.h"
#include "audio_queue.h"
#include "audio_synth.h"
#include "input.h"
#include "render.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

void test_library(void) {
    printf("Testing library...\n");
    Library lib;
    library_init(&lib);

    // Test scanning non-existent path
    assert(library_scan(&lib, "non_existent_folder") == -1);

    // Test scanning real empty directory
#if defined(_WIN32)
    mkdir("test_empty_dir");
#else
    mkdir("test_empty_dir", 0777);
#endif
    assert(library_scan(&lib, "test_empty_dir") == 0);
    remove("test_empty_dir");

    AppItem app1;
    memset(&app1, 0, sizeof(AppItem));
    snprintf(app1.title_id, sizeof(app1.title_id), "PCSG00001");
    snprintf(app1.title, sizeof(app1.title), "Uncharted Golden Abyss");
    app1.type = APP_TYPE_VITA;
    app1.section = SECTION_GAMES;
    app1.exists = true;

    AppItem app2;
    memset(&app2, 0, sizeof(AppItem));
    snprintf(app2.title_id, sizeof(app2.title_id), "NPED00001");
    snprintf(app2.title, sizeof(app2.title), "YouTube Media");
    app2.type = APP_TYPE_MEDIA;
    app2.section = SECTION_MEDIA;
    app2.exists = true;

    assert(library_add_app(&lib, &app1));
    assert(library_add_app(&lib, &app2));
    assert(lib.count == 2);
    assert(lib.games_count == 1);
    assert(lib.media_count == 1);

    AppItem *g = library_get_section_app(&lib, SECTION_GAMES, 0);
    assert(g != NULL && strcmp(g->title_id, "PCSG00001") == 0);

    AppItem *m = library_get_section_app(&lib, SECTION_MEDIA, 0);
    assert(m != NULL && strcmp(m->title_id, "NPED00001") == 0);

    library_toggle_favorite(&lib, "PCSG00001");
    assert(g->is_favorite == true);

    library_record_launch(&lib, "PCSG00001");
    assert(g->launch_count == 1);

    printf("Library test PASSED.\n");
}


/* ---- helpers para criar uma "ux0:app" falsa ---- */
static void make_dir(const char *path) {
#if defined(_WIN32)
    mkdir(path);
#else
    mkdir(path, 0777);
#endif
}

static void write_sfo(const char *path, const char *title, uint32_t num_entries) {
    unsigned char buf[128];
    memset(buf, 0, sizeof(buf));
    uint32_t magic = 0x46535000, version = 0x101, key_off = 36, data_off = 42;
    memcpy(buf + 0, &magic, 4);
    memcpy(buf + 4, &version, 4);
    memcpy(buf + 8, &key_off, 4);
    memcpy(buf + 12, &data_off, 4);
    memcpy(buf + 16, &num_entries, 4);
    uint16_t koff = 0;
    uint32_t len = (uint32_t)strlen(title) + 1, maxlen = (uint32_t)strlen(title)+1, doff = 0;
    memcpy(buf + 20, &koff, 2);
    buf[22] = 4; buf[23] = 2;
    memcpy(buf + 24, &len, 4);
    memcpy(buf + 28, &maxlen, 4);
    memcpy(buf + 32, &doff, 4);
    memcpy(buf + 36, "TITLE", 6);
    memcpy(buf + 42, title, len);
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fwrite(buf, 1, 42 + len, f);
    fclose(f);
}

static void make_app(const char *root, const char *id, const char *title, uint32_t num_entries) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", root, id);
    make_dir(path);
    snprintf(path, sizeof(path), "%s/%s/sce_sys", root, id);
    make_dir(path);
    snprintf(path, sizeof(path), "%s/%s/sce_sys/param.sfo", root, id);
    write_sfo(path, title, num_entries);
}

void test_scan_and_corrupt_sfo(void) {
    printf("Testing scan with real files, self-skip and corrupt SFO...\n");
    const char *root = "test_apps_tmp";
    make_dir(root);
    make_app(root, "PCSG00001", "Real Game", 1);
    make_app(root, "VTST00001", "VitaStation itself", 1);   /* o launcher nao pode aparecer */
    make_app(root, "BADSFO001", "ignored", 0x10000001u);    /* overflow de malloc em 32 bits */

    Library lib;
    library_init(&lib);
    assert(library_scan(&lib, root) == 1);

    bool found_self = false, found_real = false, found_bad = false;
    for (int i = 0; i < lib.count; i++) {
        if (strcmp(lib.items[i].title_id, "VTST00001") == 0) found_self = true;
        if (strcmp(lib.items[i].title_id, "PCSG00001") == 0) {
            found_real = true;
            assert(strcmp(lib.items[i].title, "Real Game") == 0);
        }
        if (strcmp(lib.items[i].title_id, "BADSFO001") == 0) {
            found_bad = true;
            assert(strcmp(lib.items[i].title, "BADSFO001") == 0);   /* SFO invalido -> usa o ID */
        }
    }
    assert(!found_self && found_real && !found_bad);

    /* uma segunda leitura preserva favoritos/estatisticas e nao duplica */
    library_toggle_favorite(&lib, "PCSG00001");
    library_record_launch(&lib, "PCSG00001");
    assert(library_scan(&lib, root) == 1);
    for (int i = 0; i < lib.count; i++) {
        if (strcmp(lib.items[i].title_id, "PCSG00001") == 0) {
            assert(lib.items[i].is_favorite && lib.items[i].launch_count == 1);
        }
    }

    char removed[256];snprintf(removed,sizeof(removed),"%s/PCSG00001/sce_sys/param.sfo",root);
    assert(remove(removed)==0);assert(library_scan(&lib,root)==0);
    printf("Scan/SFO test PASSED.\n");
}

void test_userdata_atomic(void) {
    printf("Testing recoverable userdata save/load and recovery...\n");
    Library lib;
    library_init(&lib);
    AppItem a;
    memset(&a, 0, sizeof(a));
    snprintf(a.title_id, sizeof(a.title_id), "PCSG00001");
    snprintf(a.title, sizeof(a.title), "Game");
    a.exists = true;
    a.section = SECTION_GAMES;
    a.is_favorite = true;
    a.launch_count = 7;
    a.last_played = 202610241423ULL;
    library_add_app(&lib, &a);

    remove("test_userdata.csv");
    remove("test_userdata.csv.tmp");remove("test_userdata.csv.bak");
    library_save_userdata(&lib, "test_userdata.csv");
    assert(fopen("test_userdata.csv.tmp", "r") == NULL);   /* nao sobra .tmp */

    Library lib2;
    library_init(&lib2);
    library_add_app(&lib2, &a);
    lib2.items[0].is_favorite = false;
    lib2.items[0].launch_count = 0;
    library_load_userdata(&lib2, "test_userdata.csv");
    assert(lib2.items[0].is_favorite && lib2.items[0].launch_count == 7 && lib2.items[0].last_played == 202610241423ULL);

    /* linhas corrompidas nao fazem crash nem alteram dados validos */
    FILE *f = fopen("test_userdata.csv", "w");
    assert(f != NULL);
    fputs("lixo sem virgulas\n,,,\nPCSG00001,x,y,z\n\n", f);
    fclose(f);
    library_load_userdata(&lib2, "test_userdata.csv");
    assert(lib2.items[0].launch_count == 7);

    /* recuperacao: se so existir o .tmp (escrita interrompida), e usado */
    remove("test_userdata.csv");
    f = fopen("test_userdata.csv.bak", "w");
    assert(f != NULL);
    fputs("PCSG00001,0,3,5\n", f);
    fclose(f);
    library_load_userdata(&lib2, "test_userdata.csv");
    assert(lib2.items[0].launch_count == 3 && !lib2.items[0].is_favorite);
    remove("test_userdata.csv.tmp");remove("test_userdata.csv.bak");
    printf("Userdata test PASSED.\n");
}

void test_texture_cache(void) {
    printf("Testing texture cache...\n");
    TextureCache cache;
    texture_cache_init(&cache, 1);

    Texture *t1 = texture_create_placeholder(&cache, "tex1", 256, 256, 0xFF0000FF);
    assert(t1 != NULL);
    assert(cache.current_memory_bytes == 256 * 256 * 4);

    Texture *t2 = texture_create_placeholder(&cache, "tex2", 512, 512, 0xFF00FF00);
    assert(t2 != NULL);
    assert(cache.current_memory_bytes <= cache.max_memory_bytes);

    texture_cache_clear(&cache);
    assert(cache.current_memory_bytes == 0);

    printf("Texture cache test PASSED.\n");
}

void test_ui_and_settings(void) {
    printf("Testing UI engine and settings...\n");
    Library lib;
    library_init(&lib);

    AppItem app;
    memset(&app, 0, sizeof(AppItem));
    snprintf(app.title_id, sizeof(app.title_id), "VITATEST0");
    snprintf(app.title, sizeof(app.title), "Test Game");
    app.exists = true;
    app.section = SECTION_GAMES;
    library_add_app(&lib, &app);

    UIContext ui;
    ui_init(&ui);
    assert(ui.state == UI_STATE_HOME);
    assert(ui.current_section == SECTION_GAMES);

    ui_confirm(&ui, &lib);
    assert(ui.state == UI_STATE_HOME);
    assert(launcher_last_result().status==LAUNCH_PLATFORM_UNAVAILABLE);
    ui_handle_touch(&ui,&lib,380,430);
    assert(ui.state==UI_STATE_GAME_DETAILS);

    ui_cancel(&ui);
    assert(ui.state == UI_STATE_HOME);

    ui_open_quick_menu(&ui);
    assert(ui.state == UI_STATE_QUICK_MENU);

    ui_cancel(&ui);
    assert(ui.state == UI_STATE_HOME);

    // Audio test
    audio_init();
    audio_play_sfx(SND_MOVE);
    audio_set_sfx_enabled(false);
    audio_play_sfx(SND_CONFIRM);
    audio_set_sfx_enabled(true);
    audio_shutdown();

    // Settings save & load
    ui.sound_enabled = false;
    ui.music_enabled = false;
    ui.animations_enabled = false;
    settings_save(&ui, "test_config.ini");

    UIContext ui2;
    ui_init(&ui2);
    settings_load(&ui2, "test_config.ini");
    assert(ui2.sound_enabled == false);
    assert(ui2.music_enabled == false);
    assert(ui2.animations_enabled == false);

    printf("UI and settings test PASSED.\n");
}

void test_launcher(void) {
    printf("Testing launcher...\n");
    assert(!launcher_launch_app("PCSG00001"));
    assert(!launcher_launch_app(NULL));
    assert(!launcher_launch_app(""));
    assert(!launcher_launch_app("../etc/passwd"));
    assert(!launcher_launch_app("PCSG 0001"));
    printf("Launcher test PASSED.\n");
}

static void test_hardened_cases(void) {
    puts("Testing SFO bounds and mutation sweep...");
    write_sfo("bounds.sfo","Bounds",1);
    FILE*f=fopen("bounds.sfo","rb");unsigned char original[128];size_t size=fread(original,1,128,f);fclose(f);
    char title[128],cat[32];
    assert(library_parse_param_sfo("bounds.sfo",title,sizeof(title),cat,sizeof(cat))==0);
    for(size_t cut=0;cut<size;cut++){
        f=fopen("bad.sfo","wb");assert(f);assert(fwrite(original,1,cut,f)==cut);fclose(f);
        assert(library_parse_param_sfo("bad.sfo",title,sizeof(title),cat,sizeof(cat))<0);
    }
    const int offsets[]={8,12,16,24,28,32};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++){
        unsigned char b[128];memcpy(b,original,size);memset(b+offsets[i],255,4);
        f=fopen("bad.sfo","wb");fwrite(b,1,size,f);fclose(f);
        assert(library_parse_param_sfo("bad.sfo",title,sizeof(title),cat,sizeof(cat))<0);
    }
    for(size_t i=0;i<size;i++){
        unsigned char b[128];memcpy(b,original,size);b[i]^=255;
        f=fopen("bad.sfo","wb");fwrite(b,1,size,f);fclose(f);
        library_parse_param_sfo("bad.sfo",title,sizeof(title),cat,sizeof(cat));
    }
    remove("bounds.sfo");remove("bad.sfo");
    puts("Testing categories, recent order, duplicate and disappearance...");
    Library lib;library_init(&lib);AppItem a;memset(&a,0,sizeof(a));
    a.exists=true;a.section=SECTION_GAMES;strcpy(a.title_id,"PCSG00001");a.last_played=10;a.is_favorite=true;
    assert(library_add_app(&lib,&a));assert(library_add_app(&lib,&a));assert(lib.count==1);
    strcpy(a.title_id,"HBRW00001");a.section=SECTION_HOMEBREW;a.type=APP_TYPE_HOMEBREW;a.last_played=20;a.is_favorite=false;
    assert(library_add_app(&lib,&a));
    assert(library_section_count(&lib,SECTION_GAMES)==1);
    assert(library_section_count(&lib,SECTION_HOMEBREW)==1);
    assert(library_section_count(&lib,SECTION_FAVORITES)==1);
    assert(!strcmp(library_get_section_app(&lib,SECTION_RECENT,0)->title_id,"HBRW00001"));
    lib.items[1].exists=false;assert(library_section_count(&lib,SECTION_RECENT)==1);
    puts("Testing pinning, exhausted budget and crossfade ownership...");
    TextureCache cache;texture_cache_init(&cache,1);
    Texture*t=texture_create_placeholder(&cache,"pinned",512,512,0);assert(t);t->pins=1;
    assert(!texture_create_placeholder(&cache,"blocked",1,1,0));assert(t->in_use && t->pins==1);
    assert(!texture_create_placeholder(&cache,"overflow",0xffffffff,0xffffffff,0));
    assert(!texture_cache_load(&cache,"missing.png"));
    UIContext ui;ui_init(&ui);ui.current_bg=t;ui.target_bg=t;t->pins=2;
    library_init(&lib);ui_update(&ui,&lib,&cache,.1f);assert(t->pins==1);
    for(int i=0;i<5;i++)ui_update(&ui,&lib,&cache,.1f);
    assert(!ui.current_bg && !ui.target_bg && t->pins==0);
    texture_cache_clear(&cache);
    puts("Testing two-background crossfade, rapid changes and pinned slots...");
    texture_cache_init(&cache,2);Texture *first=texture_create_placeholder(&cache,"first",128,128,0);
    Texture *second=texture_create_placeholder(&cache,"second",128,128,0);assert(first && second);
    library_init(&lib);memset(&a,0,sizeof(a));a.exists=true;a.section=SECTION_GAMES;strcpy(a.title_id,"PCSG00001");strcpy(a.pic_path,"first");library_add_app(&lib,&a);
    strcpy(a.title_id,"PCSG00002");strcpy(a.pic_path,"second");library_add_app(&lib,&a);
    ui_init(&ui);for(int i=0;i<5;i++)ui_update(&ui,&lib,&cache,.1f);assert(ui.current_bg==first && first->pins==2);
    ui_navigate_right(&ui,&lib);ui_update(&ui,&lib,&cache,.1f);
    assert(ui.current_bg==first && ui.target_bg==second && first->pins && second->pins);
    ui_navigate_left(&ui,&lib);ui_update(&ui,&lib,&cache,.1f);assert(ui.target_bg==first && second->pins==0);
    ui_navigate_right(&ui,&lib);for(int i=0;i<5;i++)ui_update(&ui,&lib,&cache,.1f);
    assert(ui.current_bg==second && second->pins==2 && first->pins==0);
    texture_cache_clear(&cache);ui.current_bg=ui.target_bg=NULL;
    texture_cache_init(&cache,1);
    for(int i=0;i<TEXTURE_CACHE_SIZE;i++){char key[20];snprintf(key,sizeof(key),"slot%d",i);Texture *slot=texture_create_placeholder(&cache,key,1,1,0);assert(slot);slot->pins=1;}
    assert(!texture_create_placeholder(&cache,"no-slot",1,1,0));texture_cache_clear(&cache);
    puts("Testing ownership of textures referenced by the frame...");
    texture_cache_init(&cache,1);Texture*drawn=texture_create_placeholder(&cache,"drawn",16,16,0);assert(drawn);
    render_begin_frame();render_draw_texture(drawn,0,0,16,16,1);assert(drawn->pins==1);
    texture_cache_evict_lru(&cache);assert(drawn->in_use);render_end_frame();assert(drawn->pins==0);
    texture_cache_evict_lru(&cache);assert(!drawn->in_use);texture_cache_clear(&cache);
    puts("Testing touch categories, settings, quick menu and loading request...");
    ui_init(&ui);ui_handle_touch(&ui,&lib,40+2*UI_TAB_WIDTH+10,30);assert(ui.current_section==SECTION_RECENT);
    ui_handle_touch(&ui,&lib,40+3*UI_TAB_WIDTH+10,30);assert(ui.current_section==SECTION_FAVORITES);
    ui_open_options(&ui);assert(ui.state==UI_STATE_GAME_SUPPORT);ui_cancel(&ui);assert(ui.state==UI_STATE_HOME);ui_open_quick_menu(&ui);ui_confirm(&ui,&lib);assert(ui.state==UI_STATE_SETTINGS);ui_navigate_down(&ui,&lib);assert(ui.settings_root_index==1);ui_confirm(&ui,&lib);assert(ui.settings_group==SETTINGS_VITASTATION);ui_navigate_down(&ui,&lib);assert(ui.settings_index==1);
    ui_confirm(&ui,&lib);assert(ui.state==UI_STATE_STORE);ui_cancel(&ui);assert(ui.state==UI_STATE_HOME);
    ui_open_quick_menu(&ui);for(int i=0;i<4;i++)ui_navigate_right(&ui,&lib);ui_confirm(&ui,&lib);
    assert(ui.rescan_requested && ui.state==UI_STATE_LOADING);ui_init(&ui);
    puts("Testing store choice, persistence and removal...");
    library_init(&lib);memset(&a,0,sizeof(a));a.exists=true;strcpy(a.title_id,"STORE0001");strcpy(a.title,"Store test");library_add_app(&lib,&a);
    ui.state=UI_STATE_STORE;ui.store_index=1;ui_confirm(&ui,&lib);assert(!strcmp(ui.store_id,"STORE0001"));
    settings_save(&ui,"store.ini");UIContext loaded;ui_init(&loaded);settings_load(&loaded,"store.ini");assert(!strcmp(loaded.store_id,ui.store_id));
    lib.count=0;ui_update(&loaded,&lib,&cache,.1f);assert(!loaded.store_id[0]);
    remove("store.ini");remove("store.ini.bak");
    puts("Testing save failure and incomplete-generation recovery...");
    assert(!settings_save(&ui,"missing-parent/config.ini"));
    ui.sound_enabled=true;assert(settings_save(&ui,"recovery.ini"));
    ui.sound_enabled=false;assert(settings_save(&ui,"recovery.ini"));
    f=fopen("recovery.ini","wb");fputs("#VITASTATION 2\nsound_enabled=0\n",f);fclose(f);
    UIContext recovered;ui_init(&recovered);settings_load(&recovered,"recovery.ini");assert(recovered.sound_enabled);
    remove("recovery.ini");remove("recovery.ini.bak");
    puts("Testing shared audio FIFO, capacity, mute and shutdown...");
    AudioQueue queue;audio_queue_init(&queue);int event=-1;
    assert(!audio_queue_pop(&queue,&event));
    for(int i=0;i<16;i++)assert(audio_queue_push(&queue,i%5));
    assert(!audio_queue_push(&queue,1));
    for(int i=0;i<16;i++){assert(audio_queue_pop(&queue,&event));assert(event==i%5);}
    audio_queue_enable(&queue,false);assert(!audio_queue_push(&queue,1));
    audio_queue_enable(&queue,true);assert(audio_queue_push(&queue,2));
    audio_queue_stop(&queue);assert(!audio_queue_pop(&queue,&event));assert(!audio_queue_push(&queue,1));
    puts("Hardened cases PASSED.");
}

static void test_revision12(void){
    puts("Testing physical Vita L/R mapping and four-category cycling...");
    assert(input_map_buttons(0x100)==BTN_L1);assert(input_map_buttons(0x200)==BTN_R1);
    assert(input_map_buttons(0x400)==BTN_L1);assert(input_map_buttons(0x800)==BTN_R1);
    assert(input_map_buttons(0x300)==(BTN_L1|BTN_R1));
    assert(input_map_buttons(0x4200)==(BTN_CROSS|BTN_R1));
    UIContext ui;ui_init(&ui);Library lib;library_init(&lib);
    for(int round=0;round<3;round++)for(int category=1;category<=SECTION_COUNT;category++){
        assert(ui_handle_category_buttons(&ui,input_map_buttons(0x200)));
        assert(ui.current_section==(AppSection)(category%SECTION_COUNT));assert(ui.selected_card_index==0);
    }
    assert(ui_handle_category_buttons(&ui,input_map_buttons(0x100)));assert(ui.current_section==SECTION_PSP_PS1);
    ui.state=UI_STATE_GAME_DETAILS;ui.scroll_offset=300;ui.selected_card_index=10;
    assert(ui_handle_category_buttons(&ui,BTN_R1));assert(ui.state==UI_STATE_HOME && ui.current_section==SECTION_GAMES && ui.scroll_offset==0);
    assert(!ui_handle_category_buttons(&ui,BTN_L1|BTN_R1));
    ui.state=UI_STATE_LOADING;assert(!ui_handle_category_buttons(&ui,BTN_R1));ui_init(&ui);
    puts("Testing relocated carousel touch selection...");
    AppItem item;memset(&item,0,sizeof(item));item.exists=true;item.section=SECTION_GAMES;
    strcpy(item.title_id,"TEST00001");library_add_app(&lib,&item);
    strcpy(item.title_id,"TEST00002");library_add_app(&lib,&item);
    ui_handle_touch(&ui,&lib,195,125);assert(ui.selected_card_index==1 && ui.state==UI_STATE_HOME);
    ui_handle_touch(&ui,&lib,195,125);assert(ui.state==UI_STATE_HOME && launcher_last_result().status==LAUNCH_PLATFORM_UNAVAILABLE);
    ui_cancel(&ui);ui_handle_touch(&ui,&lib,80,385);assert(ui.state==UI_STATE_HOME);
    puts("Testing original UI sound synthesis boundaries and distinct feedback...");
    long sums[5]={0};
    for(int effect=0;effect<5;effect++){
        unsigned frames=audio_synth_frames((SoundEffect)effect);assert(frames && frames<=VS_SFX_MAX_SAMPLES && frames%256==0);
        assert(audio_synth_sample((SoundEffect)effect,0)==0);
        assert(audio_synth_sample((SoundEffect)effect,frames)==0);
        for(unsigned frame=0;frame<frames;frame++){int value=audio_synth_sample((SoundEffect)effect,frame);assert(value>=-6000 && value<=6000);sums[effect]+=value<0?-value:value;}
        assert(sums[effect]>0);if(effect)assert(sums[effect]!=sums[effect-1]);
    }
    assert(audio_synth_frames(SND_BOOT)==122880 && audio_synth_sample(SND_BOOT,0)==0 && audio_synth_sample(SND_BOOT,122879)==0);
    long long boot_energy=0;for(unsigned i=0;i<audio_synth_frames(SND_BOOT);i++){int v=audio_synth_sample(SND_BOOT,i);assert(v>-10000 && v<10000);boot_energy+=v<0?-v:v;}assert(boot_energy>1000000);
    assert(!audio_synth_frames((SoundEffect)99));puts("Revision 1.2 tests PASSED.");
}

typedef struct {
    int installed_checks,uri_calls,flags,reply;
    bool installed;
    char id[32],uri[64];
} LaunchProbe;
static bool probe_installed(const char*id,void*context){
    LaunchProbe*p=context;p->installed_checks++;snprintf(p->id,sizeof(p->id),"%s",id);return p->installed;
}
static int probe_launch(int flags,const char*uri,void*context){
    LaunchProbe*p=context;p->uri_calls++;p->flags=flags;snprintf(p->uri,sizeof(p->uri),"%s",uri);return p->reply;
}
static bool probe_handoff(const char*id,void*context){(void)id;LaunchProbe*p=context;p->installed_checks++;return p->installed;}
static void test_revision13(void){
    LaunchProbe routing={0};routing.installed=true;bool close=true;LauncherRouteBackend route={probe_launch,probe_handoff,&routing,false};
    assert(launcher_route_app("PCSG00001",&route,&close)==0 && !close && routing.flags==0x70000 && routing.uri_calls==1 && routing.installed_checks==0);
    routing.reply=-10;assert(launcher_route_app("PCSG00001",&route,&close)==0 && close && routing.installed_checks==1);
    route.preparing=true;assert(launcher_route_app("PCSG00001",&route,&close)==-10 && !close && routing.installed_checks==1);
    route.preparing=false;routing.installed=false;assert(launcher_route_app("PCSG00001",&route,&close)==-10 && !close);
    int calls=routing.uri_calls;assert(launcher_route_app("../bad",&route,&close)<0 && routing.uri_calls==calls);

    puts("Testing valid Title IDs actually reach the launch API boundary...");
    assert(launcher_title_id_valid("PCSG00001"));assert(launcher_title_id_valid("HBRW00001"));
    assert(!launcher_title_id_valid(NULL));assert(!launcher_title_id_valid(""));
    assert(!launcher_title_id_valid("PCSG0000"));assert(!launcher_title_id_valid("PCSG000011"));
    assert(!launcher_title_id_valid("PCSG/0001"));assert(!launcher_title_id_valid("PCSG 0001"));
    LaunchProbe probe;memset(&probe,0,sizeof(probe));probe.installed=true;
    LauncherBackend backend={probe_installed,probe_launch,&probe};
    LauncherResult result=launcher_request_app("PCSG00001",&backend);
    assert(result.status==LAUNCH_ACCEPTED && result.platform_code==0);
    assert(probe.installed_checks==1 && probe.uri_calls==1 && probe.flags==0x20000);
    assert(!strcmp(probe.id,"PCSG00001") && !strcmp(probe.uri,"psgm:play?titleid=PCSG00001"));
    probe.reply=17;result=launcher_request_app("HBRW00001",&backend);
    assert(result.status==LAUNCH_ACCEPTED && result.platform_code==17 && probe.uri_calls==2);
    probe.reply=-1234;result=launcher_request_app("PCSG00001",&backend);
    assert(result.status==LAUNCH_REJECTED && result.platform_code==-1234 && probe.uri_calls==3);
    probe.installed=false;result=launcher_request_app("PCSG00001",&backend);
    assert(result.status==LAUNCH_NOT_INSTALLED && probe.uri_calls==3);
    int checks=probe.installed_checks;
    result=launcher_request_app("../bad",&backend);assert(result.status==LAUNCH_INVALID_ID);
    assert(probe.installed_checks==checks && probe.uri_calls==3);
    result=launcher_request_app("PCSG00001",NULL);assert(result.status==LAUNCH_PLATFORM_UNAVAILABLE);
    assert(!launcher_launch_app("PCSG00001"));assert(launcher_last_result().status==LAUNCH_PLATFORM_UNAVAILABLE);
    assert(!launcher_launch_app("bad"));assert(launcher_last_result().status==LAUNCH_INVALID_ID);
    puts("Testing compact row, circular action hit areas and settings shortcut...");
    Library lib;library_init(&lib);UIContext ui;ui_init(&ui);AppItem item;memset(&item,0,sizeof(item));
    item.exists=true;item.section=SECTION_GAMES;strcpy(item.title_id,"TEST00001");library_add_app(&lib,&item);
    strcpy(item.title_id,"TEST00002");library_add_app(&lib,&item);
    ui_handle_touch(&ui,&lib,150,125);assert(ui.selected_card_index==1);
    ui_handle_touch(&ui,&lib,322,433);assert(lib.items[1].is_favorite);
    ui_handle_touch(&ui,&lib,382,433);assert(ui.state==UI_STATE_GAME_DETAILS);
    ui_cancel(&ui);ui_handle_touch(&ui,&lib,751,30);assert(ui.state==UI_STATE_SETTINGS);
    ui_cancel(&ui);ui_handle_touch(&ui,&lib,707,30);assert(ui.state==UI_STATE_QUICK_MENU);
    puts("Testing cinematic background lifetime through the cover renderer...");
    TextureCache cache;texture_cache_init(&cache,1);Texture*t=texture_create_placeholder(&cache,"cover",128,256,0);assert(t);
    render_begin_frame();render_draw_texture_cover(t,0,0,960,544,1);assert(t->pins==1);
    texture_cache_evict_lru(&cache);assert(t->in_use);render_end_frame();assert(t->pins==0);
    texture_cache_clear(&cache);
    puts("Revision 1.3 regression tests PASSED (injected API, not a hardware launch).");
}

static void test_store_volume_settings(void){
    puts("Testing repeated preferences save/readback and persisted store selection...");
    UIContext ui;ui_init(&ui);Library lib;library_init(&lib);AppItem app;memset(&app,0,sizeof(app));
    app.exists=true;strcpy(app.title_id,"STORE0001");strcpy(app.title,"My installed store");library_add_app(&lib,&app);
    for(int i=0;i<4;i++){
        snprintf(ui.store_id,sizeof(ui.store_id),"STORE0001");ui.sound_enabled=i%2!=0;ui.animations_enabled=i%2==0;
        assert(settings_save(&ui,"preferences.ini"));UIContext restored;ui_init(&restored);settings_load(&restored,"preferences.ini");
        assert(!strcmp(restored.store_id,"STORE0001") && restored.sound_enabled==ui.sound_enabled && restored.animations_enabled==ui.animations_enabled);
        ui_open_store_picker(&restored,&lib);assert(restored.store_index==1);
    }
    assert(settings_save(&ui,VS_CONFIG_PATH));UIContext restarted;ui_init(&restarted);settings_load(&restarted,VS_CONFIG_PATH);
    ui_open_store_picker(&restarted,&lib);assert(restarted.store_index==1);ui_confirm(&restarted,&lib);
    assert(restarted.state==UI_STATE_HOME && restarted.current_section==SECTION_STORE && !strcmp(restarted.store_id,"STORE0001"));
    assert(ui_selected_app(&restarted,&lib)==&lib.items[0]);
    ui_confirm(&restarted,&lib);assert(launcher_last_result().status==LAUNCH_PLATFORM_UNAVAILABLE && lib.items[0].launch_count==0);
    TextureCache cache;texture_cache_init(&cache,1);Library unavailable;library_init(&unavailable);
    ui_update(&restarted,&unavailable,&cache,.1f);assert(!strcmp(restarted.store_id,"STORE0001"));
    unavailable.scan_complete=true;ui_update(&restarted,&unavailable,&cache,.1f);assert(!restarted.store_id[0]);
    ui_select_tab(&ui,SECTION_PSP_PS1);assert(ui_handle_category_buttons(&ui,BTN_R1) && ui.current_section==SECTION_GAMES);
    puts("Testing real-value volume HUD observation and expiry...");
    ui_init(&ui);assert(ui.system_volume==-1);ui_observe_volume(&ui,10,false);assert(ui.volume_timer==0);
    ui_observe_volume(&ui,11,false);assert(ui.volume_timer==2 && ui.system_volume==11);
    ui_update(&ui,&lib,&cache,.1f);assert(ui.volume_timer>1.8f && ui.volume_timer<2);
    ui_observe_volume(&ui,0,false);assert(ui.system_volume==0 && ui.volume_timer==2);
    ui_observe_volume(&ui,30,true);assert(ui.system_volume==30 && ui.volume_timer==2);
    ui_observe_volume(&ui,-1,true);ui_observe_volume(&ui,31,true);assert(ui.system_volume==30);
    for(int i=0;i<30;i++)ui_update(&ui,&lib,&cache,.1f);assert(ui.volume_timer==0);
    assert(input_map_buttons(0x100000)==BTN_VOLUME_UP && input_map_buttons(0x200000)==BTN_VOLUME_DOWN);
    texture_cache_clear(&cache);remove("preferences.ini");remove("preferences.ini.bak");
    puts("Testing startup logo lifetime and skip without launching...");
    ui_init(&ui);ui.state=UI_STATE_SPLASH;
    assert(!ui_handle_category_buttons(&ui,BTN_R1));
    for(int i=0;i<10;i++)ui_update(&ui,&lib,&cache,.1f);
    assert(ui.state==UI_STATE_SPLASH);
    for(int i=0;i<10;i++)ui_update(&ui,&lib,&cache,.1f);
    assert(ui.state==UI_STATE_HOME);
    ui.state=UI_STATE_SPLASH;ui_confirm(&ui,&lib);assert(ui.state==UI_STATE_HOME);
    puts("Store/settings/volume tests PASSED.");
}

int main(void) {
    printf("=== RUNNING VITASTATION HOST VERIFICATION SUITE ===\n");
    printf("[Note: Host verification harness tests C engine logic without PS Vita hardware dependencies]\n");
    test_library();
    test_scan_and_corrupt_sfo();
    test_userdata_atomic();
    test_texture_cache();
    test_ui_and_settings();
    test_launcher();
    test_hardened_cases();
    test_revision12();
    test_revision13();
    test_store_volume_settings();
    printf("=== ALL HOST TESTS PASSED SUCCESSFULLY ===\n");
    return 0;
}
