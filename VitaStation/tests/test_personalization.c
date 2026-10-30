#include "personalization.h"
#include "settings.h"
#include "settings_menu.h"
#include "library_cache.h"
#include "launcher.h"
#include "theme.h"
#include "input.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Library lib,restored;static UIContext u,v;
int main(void){
 library_init(&lib);AppItem a={0};a.exists=true;a.type=APP_TYPE_SYSTEM;a.section=SECTION_VITA;strcpy(a.title_id,"NPXS10004");strcpy(a.title,"Photos");assert(library_add_app(&lib,&a));
 a.type=APP_TYPE_VITA;a.section=SECTION_GAMES;a.installed_at=20;strcpy(a.title_id,"PCSE00001");strcpy(a.title,"A game");assert(library_add_app(&lib,&a));
 a.type=APP_TYPE_HOMEBREW;a.section=SECTION_HOMEBREW;a.installed_at=30;strcpy(a.title_id,"HBRW00001");strcpy(a.title,"Z homebrew");assert(library_add_app(&lib,&a));
 assert(library_section_count(&lib,SECTION_VITA)==1 && library_section_count(&lib,SECTION_GAMES)==1);
 ui_init(&u);u.state=UI_STATE_HOME;ui_handle_touch(&u,&lib,40+UI_TAB_WIDTH+8,30);assert(u.current_section==SECTION_VITA);
 assert(ui_handle_category_buttons(&u,BTN_R1) && u.current_section==SECTION_HOMEBREW);
 u.state=UI_STATE_SETTINGS;u.settings_group=SETTINGS_VITASTATION;u.settings_index=15;ui_confirm(&u,&lib);assert(u.state==UI_STATE_SYSTEM_APPS && u.native_count==3 && !u.exit_requested);
 /* Alphabetical picker: a game first, fixed Photos second. No app is opened. */
 ui_confirm(&u,&lib);assert(!u.exit_requested && library_vita_contains(&lib,"PCSE00001"));assert(library_section_count(&lib,SECTION_VITA)==2 && library_section_count(&lib,SECTION_GAMES)==1);
 library_load_vita(&restored,VS_VITA_PATH);assert(library_vita_contains(&restored,"PCSE00001"));
 assert(library_cache_save(&lib,"apps.cache") && library_cache_load(&restored,"apps.cache"));assert(library_section_count(&restored,SECTION_VITA)==1);library_load_vita(&restored,VS_VITA_PATH);assert(library_section_count(&restored,SECTION_VITA)==2);
 u.native_index=1;ui_confirm(&u,&lib);assert(library_section_count(&lib,SECTION_VITA)==2 && !u.exit_requested); /* Default apps fixed. */
 u.native_index=0;ui_confirm(&u,&lib);assert(!library_vita_contains(&lib,"PCSE00001"));
 AppItem swap=lib.items[0];lib.items[0]=lib.items[2];lib.items[2]=swap;u.native_index=0;ui_confirm(&u,&lib);assert(library_vita_contains(&lib,"PCSE00001") && !library_vita_contains(&lib,"HBRW00001"));ui_confirm(&u,&lib);assert(!library_vita_contains(&lib,"PCSE00001"));
 /* Empty touch rows must never toggle the final real row. */
 u.native_index=0;system_ui_touch(&u,&lib,80,390);assert(!library_vita_contains(&lib,"PCSE00001"));
 assert(!library_vita_set(&lib,"../bad",true) && !library_vita_set(&lib,"VTST00001",true));
 ui_cancel(&u);u.settings_index=18;ui_confirm(&u,&lib);assert(u.state==UI_STATE_PERSONALIZATION && !ui_handle_category_buttons(&u,BTN_R1));
 ui_confirm(&u,&lib);assert(u.state==UI_STATE_ASSET_PICKER);ui_confirm(&u,&lib);assert(u.environment_theme==5 && u.state==UI_STATE_PERSONALIZATION);
 ui_init(&v);settings_load(&v,VS_CONFIG_PATH);assert(v.environment_theme==5);
 for(int i=5;i<VS_THEME_COUNT;i++){assert(strchr(theme_environment_name(i),'|'));assert(theme_canvas_color(i,0)!=theme_canvas_color(i,544));for(int j=5;j<i;j++)assert(theme_canvas_color(i,0)!=theme_canvas_color(j,0));}
 strcpy(u.wallpaper_path,"ux0:data/VitaStation/wallpapers/my image.png");strcpy(u.font_path,"app0:assets/fonts/Rajdhani-Medium.ttf");strcpy(u.lock_path,"ux0:picture/ALL/sample.png");assert(personalization_save(&u,"appearance.test"));personalization_load(&v,"appearance.test");assert(!strcmp(u.wallpaper_path,v.wallpaper_path) && !strcmp(u.font_path,v.font_path) && !strcmp(u.lock_path,v.lock_path));
 /* Last committed appearance remains available after an interrupted write. */
 assert(personalization_save(&u,"appearance.test"));FILE*f=fopen("appearance.test","wb");assert(f);fputs("#VITASTATION 2\nwallpaper=broken",f);fclose(f);v.wallpaper_path[0]=0;personalization_load(&v,"appearance.test");assert(!strcmp(v.wallpaper_path,u.wallpaper_path));
 assert(!personalization_image_valid("missing.png",false));
 puts("Vita tab/order, settings-only membership, fixed defaults, persistence/cache compatibility, empty touch, 10 distinct themes, font/wallpaper paths and interrupted-save recovery passed.");
 return 0;
}
