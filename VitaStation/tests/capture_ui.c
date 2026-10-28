/* Diagnostic fixture only: never included in the Vita executable. */
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Library lib;static TextureCache cache;static UIContext ui;
int main(int argc,char**argv){
    library_init(&lib);texture_cache_init(&cache,8);ui_init(&ui);
    int state=argc>1?atoi(argv[1]):0;
    if(state!=6){for(int i=0;i<6;i++){AppItem a;memset(&a,0,sizeof(a));a.exists=true;a.section=SECTION_GAMES;snprintf(a.title_id,sizeof(a.title_id),"TEST%05d",i);snprintf(a.title,sizeof(a.title),"TESTE | Aplicacao %d",i+1);library_add_app(&lib,&a);}}
    ui.state=state==6?UI_STATE_HOME:(UIState)state;ui.toast_timer=0;
    if(state==7){ui.state=UI_STATE_HOME;ui.current_section=SECTION_STORE;snprintf(ui.store_id,sizeof(ui.store_id),"TEST00000");snprintf(lib.items[0].title,sizeof(lib.items[0].title),"TESTE | Loja instalada");}
    if(state==9){ui.state=UI_STATE_SPLASH;ui.splash_time=.5f;}
    if(state==8){ui.state=UI_STATE_HOME;ui_observe_volume(&ui,18,true);}
    snprintf(ui.clock_str,sizeof(ui.clock_str),"--:--");ui.battery_percent=-1;
    ui_render(&ui,&lib,&cache);texture_cache_clear(&cache);return 0;
}
