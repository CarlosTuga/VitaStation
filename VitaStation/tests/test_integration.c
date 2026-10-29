#include "plugins.h"
#include "library_cache.h"
#include "bgdl_payload.h"
#include <stddef.h>
#include "catalog.h"
#include "system_controls.h"
#include "ui.h"
#include "store_ui.h"
#include "autoboot.h"
#include "system_ui.h"
#include "settings_menu.h"
#include "library.h"
#include "dlc_plan.h"
#include "../plugins/VitaStationBoot/gesture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
static int volume=12,read_calls=0,read_error=0,write_error=0,maximum=65536,brightness=-1;
static int dlc_calls=0,stop_after_first=0;
static bool install_dlc_test(const CatalogPackage*p,void*context){(void)context;assert(p->type==CATALOG_DLCS && !strcmp(p->title_id,"PCSE00001"));return ++dlc_calls!=2;}
static bool cancel_dlc_test(void*context){(void)context;return stop_after_first && dlc_calls>=1;}
static int read_volume(int*v){++read_calls;*v=volume;return read_error;}
static int write_volume(int v){if(!write_error)volume=v;return write_error;}
static int max_brightness(int*v){*v=maximum;return 0;}
static int write_brightness(int v){brightness=v;return write_error;}
static int saved_brightness=32768,save_error=0;
static int read_saved_brightness(int*v){*v=saved_brightness;return 0;}
static int save_brightness_test(int v){if(!save_error)saved_brightness=v;return save_error;}
static void file(const char*name,const char*text){FILE*f=fopen(name,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));}
int main(int argc,char**argv){
    assert(sizeof(BgdlPayload)==0xdc0 && offsetof(BgdlPayload,url)==0x70 && offsetof(BgdlPayload,license_path)==0xcaa);
    BgdlPayload wire;assert(bgdl_payload_build(&wire,22,"Homebrew PKG","https://example.org/app.pkg","ux0:data/license.rif","ux0:data/icon.png"));
    assert(wire.type[0]==22 && wire.type[1]==22 && !strcmp(wire.url,"https://example.org/app.pkg"));
    assert(!bgdl_payload_build(&wire,99,"x","https://example.org/a","",""));assert(!bgdl_payload_build(&wire,22,"x","file://unsafe","",""));
    char long_url[2050];memset(long_url,'x',sizeof(long_url));memcpy(long_url,"https://",8);long_url[sizeof(long_url)-1]=0;assert(!bgdl_payload_build(&wire,22,"x",long_url,"",""));
    char a[4096],b[4096];const char*config="# Keep me\r\n*main\r\nuser.suprx\r\n*KERNEL\r\nur0:tai/nonpdrm.skprx\r\n*ALL\r\nur0:tai/other.suprx\r\n";
    assert(plugins_config_add(config,"*KERNEL","ur0:tai/VitaStation/nonpdrm.skprx",a,sizeof(a))==0 && !strcmp(a,config));
    assert(plugins_config_add(config,"*KERNEL","ur0:tai/VitaStation/nopsmdrm.skprx",a,sizeof(a))==1);
    assert(strstr(a,"# Keep me\r\n*main\r\nuser.suprx\r\n"));assert(strstr(a,"nopsmdrm.skprx\n*ALL"));
    assert(plugins_config_add(a,"*ALL","ur0:tai/VitaStation/NoPspEmuDrm_user.suprx",b,sizeof(b))==1);
    assert(plugins_config_add(b,"*ALL","ur0:tai/VitaStation/NoPspEmuDrm_user.suprx",a,sizeof(a))==0 && !strcmp(a,b));
    assert(plugins_config_add(config,"*ALL","ur0:tai/nonpdrm.skprx",a,sizeof(a))==-1);
    assert(plugins_config_add(config,"*KERNEL","ur0:tai/evil\n*ALL",a,sizeof(a))==-1);
    assert(plugins_config_add(config,"*KERNEL","ur0:tai/new.skprx",a,10)==-1);
    assert(plugins_psp_enable("other.prx 1\n","ms0:/seplugins/npdrm_free.prx",a,sizeof(a))==1);
    assert(strstr(a,"other.prx 1\nms0:/seplugins/npdrm_free.prx 1\n"));
    assert(plugins_psp_enable(a,"ms0:/seplugins/npdrm_free.prx",b,sizeof(b))==0);
    assert(plugins_psp_enable("ms0:/seplugins/npdrm_free.prx 0\n","ms0:/seplugins/npdrm_free.prx",a,sizeof(a))==1 && strstr(a,"prx 1\n"));
    assert(plugins_psp_enable(a,"ms0:/seplugins/npdrm_free_mod.prx",b,sizeof(b))==-1);
    PluginEntry entries[8];size_t number=plugins_config_entries(config,entries,8);assert(number==2 && entries[0].enabled);
    assert(plugins_config_toggle(config,&entries[0],false,a,sizeof(a))==1);
    assert(strstr(a,"#VITASTATION_DISABLED ur0:tai/nonpdrm.skprx"));
    assert(plugins_config_entries(a,entries,8)==2 && !entries[0].enabled);
    assert(plugins_config_toggle(a,&entries[0],true,b,sizeof(b))==1 && !strstr(b,"#VITASTATION_DISABLED"));
    plugins_config_entries("*KERNEL\nur0:tai/storagemgr.skprx\n",entries,8);assert(entries[0].protected_entry);
    assert(plugins_config_toggle("*KERNEL\nur0:tai/storagemgr.skprx\n",&entries[0],false,b,sizeof(b))==-1);
    assert(!autoboot_marker_read("boot-test"));assert(autoboot_marker_write("boot-test",false));
    for(int i=0;i<5;i++){assert(autoboot_marker_write("boot-test",true) && autoboot_marker_read("boot-test"));assert(autoboot_marker_write("boot-test",false) && !autoboot_marker_read("boot-test"));}
    file("boot-test","VTST00001\nextra");assert(!autoboot_marker_read("boot-test"));assert(autoboot_marker_write("boot-test",true));assert(autoboot_marker_write("boot-test",false));
    VitaStationPSGesture gesture={0};
    assert(vs_ps_gesture(&gesture,1,100000)==VS_PS_NONE);assert(vs_ps_gesture(&gesture,0,160000)==VS_PS_NONE);assert(vs_ps_gesture(&gesture,0,510000)==VS_PS_SINGLE);assert(vs_ps_gesture(&gesture,0,600000)==VS_PS_NONE);
    memset(&gesture,0,sizeof(gesture));vs_ps_gesture(&gesture,1,100000);vs_ps_gesture(&gesture,0,160000);vs_ps_gesture(&gesture,1,240000);assert(vs_ps_gesture(&gesture,0,290000)==VS_PS_DOUBLE);assert(vs_ps_gesture(&gesture,0,900000)==VS_PS_NONE);
    memset(&gesture,0,sizeof(gesture));vs_ps_gesture(&gesture,1,100000);assert(vs_ps_gesture(&gesture,1,700000)==VS_PS_NONE);assert(vs_ps_gesture(&gesture,0,800000)==VS_PS_NONE);assert(vs_ps_gesture(&gesture,0,1500000)==VS_PS_NONE);
    SystemControlBackend backend={read_volume,write_volume,max_brightness,write_brightness,NULL,NULL};
    SystemControlResult r=system_control_apply(SYS_VOLUME,20,&backend);assert(r.status==SYS_APPLIED && volume==20 && read_calls==2);
    r=system_control_apply(SYS_VOLUME,31,&backend);assert(r.status==SYS_INVALID && volume==20);
    read_error=-1;r=system_control_apply(SYS_VOLUME,21,&backend);assert(r.status==SYS_READ_FAILED && volume==20);read_error=0;
    write_error=-2;r=system_control_apply(SYS_VOLUME,21,&backend);assert(r.status==SYS_WRITE_FAILED && volume==20);write_error=0;
    r=system_control_apply(SYS_BRIGHTNESS,50,&backend);assert(r.status==SYS_APPLIED_UNVERIFIED && brightness==32768);
    r=system_control_apply(SYS_BRIGHTNESS,0,&backend);assert(r.status==SYS_INVALID && brightness==32768);
    backend.read_brightness=read_saved_brightness;backend.save_brightness=save_brightness_test;
    r=system_control_apply(SYS_BRIGHTNESS,75,&backend);assert(r.status==SYS_BRIGHTNESS_SAVED && brightness==49152 && saved_brightness==49152);
    save_error=-1;r=system_control_apply(SYS_BRIGHTNESS,25,&backend);assert(r.status==SYS_WRITE_FAILED && brightness==49152 && saved_brightness==49152);save_error=0;
    assert(system_control_apply(SYS_VOLUME,5,NULL).status==SYS_UNAVAILABLE);
    assert(system_feature_count()==96 && !system_feature_get(96));
    puts("Plugin configuration and native-control error handling passed.");
    if(argc>1){
        StoreCatalog catalog;catalog_init(&catalog);
        for(int type=0;type<CATALOG_HOMEBREW;type++){
            assert(catalog_load_root(&catalog,argv[1],(CatalogType)type));assert(catalog.count>0);
            unsigned downloadable=0,unavailable=0;CatalogPackage package;char error[256];
            for(size_t i=0;i<catalog.count;i++){if(catalog_get_package(&catalog,i,&package,error,sizeof(error)))++downloadable;else ++unavailable;}
            printf("%s: %zu items, %u downloadable metadata, %u unavailable.\n",catalog_type_name((CatalogType)type),catalog.count,downloadable,unavailable);
        }
        catalog_clear(&catalog);
    }
    mkdir("catalog-fixture");
    file("catalog-fixture/games.json","[{\"title_id\":\"BREW00001\",\"name\":\"Homebrew Test\",\"region\":\"EU\",\"url\":\"https://example.org/homebrew.vpk\"}]");
    StoreCatalog c;catalog_init(&c);assert(catalog_load_root(&c,"catalog-fixture",CATALOG_GAMES) && c.source_json && c.count==1);
    CatalogPackage pkg;char error[256];assert(catalog_get_package(&c,0,&pkg,error,sizeof(error)) && pkg.vpk);
    file("catalog-fixture/titles_psvgames.tsv","Title ID\tRegion\tName\tPKG direct link\tzRIF\tContent ID\tDate\tOriginal Name\tFile Size\tSHA256\nPCSE00001\tUSA\tTest Game\thttps://example.org/a.pkg\t\tUP0000-PCSE00001_00-TESTPACKAGE00001\t\t\t1024\t\n");
    assert(catalog_load_root(&c,"catalog-fixture",CATALOG_GAMES) && !c.source_json && c.count==1);
    assert(catalog_get_package(&c,0,&pkg,error,sizeof(error)) && !pkg.vpk && pkg.size_bytes==1024);
    file("catalog-fixture/titles_psvdlcs.tsv","Title ID\tRegion\tName\tURL\tzRIF\tContent ID\tDate\tSize\tSHA256\n"
        "PCSE00001\tUSA\tDLC A\thttps://example.org/a.pkg\t\tUP0000-PCSE00001_00-DLC0000000000001\t\t100\t\n"
        "PCSB00001\tEUR\tWrong region\thttps://example.org/eu.pkg\t\tEP0000-PCSB00001_00-DLC0000000000001\t\t100\t\n"
        "PCSE00001\tUSA\tDLC B\thttps://example.org/b.pkg\t\tUP0000-PCSE00001_00-DLC0000000000002\t\t100\t\n"
        "PCSE00001\tUSA\tDuplicate A\thttps://example.org/a.pkg\t\tUP0000-PCSE00001_00-DLC0000000000001\t\t100\t\n"
        "PCSE00001\tUSA\tUnavailable\tMISSING\t\tUP0000-PCSE00001_00-DLC0000000000003\t\t100\t\n");
    StoreCatalog dlcs;catalog_init(&dlcs);assert(catalog_load_matching_root(&dlcs,"catalog-fixture",CATALOG_DLCS,"PCSE00001") && dlcs.count==4);
    DLCPlanResult plan=dlc_plan_run(&pkg,&dlcs,install_dlc_test,cancel_dlc_test,NULL);assert(plan.installed==1 && plan.failed==1 && plan.unavailable==1 && plan.duplicates==1 && !plan.canceled && dlc_calls==2);
    dlc_calls=0;stop_after_first=1;plan=dlc_plan_run(&pkg,&dlcs,install_dlc_test,cancel_dlc_test,NULL);assert(plan.installed==1 && plan.canceled && dlc_calls==1);catalog_clear(&dlcs);
    static Library library;library_init(&library);AppItem app;memset(&app,0,sizeof(app));app.exists=true;
    strcpy(app.title_id,"PCSE00001");strcpy(app.category_str,"gd");strcpy(app.content_id,"UP0000-PCSE00001_00-TESTPACKAGE00001");library_add_app(&library,&app);
    strcpy(app.title_id,"HBRW00001");strcpy(app.content_id,"UP0000-HBRW00001_00-FAKEPACKAGE00001");library_add_app(&library,&app);
    library_classify_catalog(&library,"catalog-fixture/titles_psvgames.tsv",NULL);
    assert(library_section_count(&library,SECTION_GAMES)==1 && library_section_count(&library,SECTION_HOMEBREW)==1);
    strcpy(library.items[0].content_id,"UP0000-PCSE00001_00-FAKEPACKAGE00001");library_classify_catalog(&library,"catalog-fixture/titles_psvgames.tsv",NULL);assert(library_section_count(&library,SECTION_GAMES)==0);
    library.items[0].is_favorite=true;library.items[0].launch_count=7;
    assert(library_cache_save(&library,"library-cache.bin"));static Library cached;library_init(&cached);assert(library_cache_load(&cached,"library-cache.bin") && cached.count==library.count && cached.scan_complete && cached.items[0].is_favorite && cached.items[0].launch_count==7);
    FILE*badcache=fopen("library-cache.bin","r+b");assert(badcache);fputc('X',badcache);fclose(badcache);int old_count=cached.count;assert(!library_cache_load(&cached,"library-cache.bin") && cached.count==old_count);
    assert(library_cache_save(&library,"library-cache.bin") && library_cache_load(&cached,"library-cache.bin"));
    file("library-cache.bin","truncated");assert(!library_cache_load(&cached,"library-cache.bin") && cached.count==old_count);
    remove("starts.txt");assert(library_cache_rebuild_due_boot(false,"starts.txt",1000));
    library_cache_reset_counter("starts.txt");
    assert(!library_cache_rebuild_due_boot(true,"starts.txt",1000));
    assert(!library_cache_rebuild_due_boot(true,"starts.txt",1002)); /* same boot, RTC rounding */
    assert(!library_cache_rebuild_due_boot(true,"starts.txt",2000)); /* first new boot */
    assert(!library_cache_rebuild_due_boot(true,"starts.txt",2000)); /* reopening does not count */
    assert(library_cache_rebuild_due_boot(true,"starts.txt",3000)); /* second boot */
    assert(library_cache_rebuild_due_boot(true,"starts.txt",3000)); /* failed scan retries */
    library_cache_reset_counter("starts.txt");assert(!library_cache_rebuild_due_boot(true,"starts.txt",3000));
    puts("BGDL wire layout, metadata bounds and startup snapshot recovery passed.");
    UIContext u;ui_init(&u);u.catalog=&c;u.current_section=SECTION_STORE;
    assert(store_ui_input(&u,STORE_UI_CONFIRM));assert(u.state==UI_STATE_HOME && strstr(u.toast_msg,"PS Vita"));
    assert(store_ui_input(&u,STORE_UI_INFO) && u.state==UI_STATE_CATALOG_DETAILS);
    assert(store_ui_input(&u,STORE_UI_CANCEL) && u.state==UI_STATE_HOME);
    ui_open_power_menu(&u);assert(u.state==UI_STATE_POWER && u.power_index==2);assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_HOME);
    static TextureCache cache;texture_cache_init(&cache,8);u.catalog=NULL;u.state=UI_STATE_SETTINGS;
    for(int index=0;index<(int)system_feature_count()+UI_LOCAL_SETTINGS_COUNT;index++){u.settings_index=index;u.settings_group=settings_group_for(index);ui_render(&u,&library,&cache);}
    for(size_t index=0;index<system_feature_count();index++){
        const SystemFeature*f=system_feature_get(index);assert(system_feature_reason(f));
        if(f->control==SYS_ACTION_LIBRARY){u.state=UI_STATE_SETTINGS;u.settings_index=(int)index+UI_LOCAL_SETTINGS_COUNT;u.settings_group=settings_group_for(u.settings_index);assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_HOME && u.current_section==SECTION_GAMES);}
        if(f->control==SYS_ACTION_POWER){u.state=UI_STATE_SETTINGS;u.settings_index=(int)index+UI_LOCAL_SETTINGS_COUNT;u.settings_group=settings_group_for(u.settings_index);assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_POWER);}
    }
    u.state=UI_STATE_SETTINGS;u.settings_index=14;u.settings_group=SETTINGS_VITASTATION;
    assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_UNINSTALL && u.power_index==0);
    ui_render(&u,&library,&cache);
    assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_SETTINGS);
    assert(system_ui_input(&u,&library,SYSTEM_UI_CONFIRM) && u.state==UI_STATE_UNINSTALL);
    assert(system_ui_input(&u,&library,SYSTEM_UI_RIGHT) && u.power_index==1);
    assert(system_ui_input(&u,&library,SYSTEM_UI_CANCEL) && u.state==UI_STATE_SETTINGS);
    int total=(int)system_feature_count()+UI_LOCAL_SETTINGS_COUNT,seen[256]={0};
    assert(total<256 && settings_group_count(SETTINGS_CONSOLE)>0 && settings_group_count(SETTINGS_VITASTATION)>0);
    for(int group=SETTINGS_CONSOLE;group<=SETTINGS_VITASTATION;group++){
        int n=settings_group_count(group);
        for(int pos=0;pos<n;pos++){int raw=settings_group_item(group,pos);assert(raw>=0 && raw<total && !seen[raw]++);assert(settings_group_for(raw)==group && settings_group_position(group,raw)==pos);}
        assert(settings_group_item(group,n)==-1);
    }
    for(int raw=0;raw<total;raw++){const SystemFeature*f=raw>=UI_LOCAL_SETTINGS_COUNT?system_feature_get((size_t)(raw-UI_LOCAL_SETTINGS_COUNT)):NULL;assert(seen[raw]==(f && f->control<0?0:1));}
    for(size_t i=0;i<system_feature_count();i++){
        const SystemFeature*f=system_feature_get(i);int group=settings_group_for((int)i+UI_LOCAL_SETTINGS_COUNT);
        if(f->control>=SYS_ACTION_LIBRARY)assert(group==(f->control==SYS_ACTION_SUSPEND?SETTINGS_CONSOLE:SETTINGS_VITASTATION));
    }
    u.state=UI_STATE_HOME;settings_menu_open(&u);assert(u.settings_group==SETTINGS_ROOT);
    ui_navigate_right(&u,&library);ui_confirm(&u,&library);assert(u.settings_group==SETTINGS_VITASTATION && u.settings_index==0);
    int last=settings_group_item(SETTINGS_VITASTATION,settings_group_count(SETTINGS_VITASTATION)-1);
    for(int i=0;i<total+2;i++)ui_navigate_down(&u,&library);
    assert(u.settings_index==last);ui_cancel(&u);assert(u.state==UI_STATE_SETTINGS && u.settings_group==SETTINGS_ROOT && u.settings_root_index==1);
    ui_cancel(&u);assert(u.state==UI_STATE_HOME);
    settings_menu_open(&u);assert(settings_menu_touch(&u,&library,100,250) && u.settings_group==SETTINGS_CONSOLE);
    assert(settings_menu_touch(&u,&library,20,90) && u.settings_group==SETTINGS_ROOT);
    assert(settings_menu_touch(&u,&library,600,250) && u.settings_group==SETTINGS_VITASTATION);
    for(size_t i=0;i<system_feature_count();i++)if(system_feature_get(i)->control==SYS_ACTION_REFRESH){u.settings_index=(int)i+UI_LOCAL_SETTINGS_COUNT;u.rescan_requested=false;ui_confirm(&u,&library);assert(u.rescan_requested && u.state==UI_STATE_HOME);u.state=UI_STATE_SETTINGS;}
    puts("Settings separation: supported entries grouped and unimplemented entries hidden, filtered navigation, nested back, touch and manual database rebuild passed.");
    texture_cache_clear(&cache);
    catalog_clear(&c);
    mkdir("region-fixture");
    file("region-fixture/games.json","[{\"title_id\":\"ASIA00001\",\"name\":\"Asian\",\"region\":\"Asian\"},{\"title_id\":\"ASIA00002\",\"name\":\"Asia\",\"region\":\"asia\"},{\"title_id\":\"TEST00001\",\"name\":\"Europe\",\"region\":\"EU\"},{\"title_id\":\"TEST00002\",\"name\":\"Japan\",\"region\":\"JP\"}]");
    assert(catalog_load_root(&c,"region-fixture",CATALOG_GAMES) && c.count==1 && c.skipped==3);
    assert(!strcmp(c.items[0].region,"EU"));
    file("region-fixture/titles_psvgames.tsv","Title ID\tRegion\tName\nASIA00001\tASIA\tFiltered\nASIA00002\tASN\tFiltered ASN\nTEST00001\tEU\tEuropean\nTEST00002\tJP\tJapanese\n");
    assert(catalog_load_root(&c,"region-fixture",CATALOG_GAMES) && c.count==1 && c.skipped==3);
    assert(!strcmp(c.items[0].region,"EU"));
    file("region-fixture/titles_psvgames.tsv","Title ID\tRegion\tName\nTEST00001\tEurope\tEuropean\nTEST00002\tUSA\tAmerican\nTEST00003\tJP\tJapanese\nTEST00004\t\tUnknown\n");
    assert(catalog_load_root(&c,"region-fixture",CATALOG_GAMES) && c.count==2 && c.skipped==2);
    assert(!strcmp(c.items[0].region,"EU") && !strcmp(c.items[1].region,"US"));
    catalog_clear(&c);puts("Region filter passed for TSV and JSON; only EU and US admitted.");
    remove("catalog-fixture/titles_psvgames.tsv");remove("catalog-fixture/games.json");
    puts("Catalog formats, TSV priority, metadata extraction and X/info separation passed.");return 0;
}

