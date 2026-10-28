#include "psp_media.h"
#include "library_cache.h"
#include "installation_watch.h"
#include "theme.h"
#include "settings.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static Library lib,cached;
int main(void){
 assert(library_date(1970,1,1,0,0,0)==0 && library_date(2024,1,1,0,0,0)==1704067200 && !library_date(2024,13,1,0,0,0));
 AppItem a;memset(&a,0,sizeof(a));strcpy(a.title_id,"TEST00001");assert(psp_read_metadata("games/PS1/EBOOT.PBP",&a));assert(a.type==APP_TYPE_PS1 && !strcmp(a.title,"Test PS1") && !strcmp(a.game_id,"SLUS00980"));
 memset(&a,0,sizeof(a));strcpy(a.title_id,"TEST00002");assert(psp_read_metadata("games/PSP/EBOOT.PBP",&a));assert(a.type==APP_TYPE_PSP && !strcmp(a.title,"Test PSP"));
 memset(&a,0,sizeof(a));strcpy(a.title_id,"TEST00003");assert(psp_read_metadata("games/ISO/Test.iso",&a) && a.type==APP_TYPE_PSP && !strcmp(a.title,"Test ISO"));
 assert(!psp_read_metadata("games/bad.pbp",&a));
 library_init(&lib);psp_media_scan(&lib,"games");assert(lib.count==3 && library_section_count(&lib,SECTION_PSP_PS1)==3);
 psp_media_scan(&lib,"games");assert(lib.count==3);
 for(int i=0;i<3;i++){assert(lib.items[i].media_raw && lib.items[i].media_path[0] && lib.items[i].installed_at>0);lib.items[i].installed_at=100+(unsigned)i*100;lib.items[i].last_played=300-(unsigned)i*100;lib.items[i].is_favorite=true;}
 assert(library_get_section_app(&lib,SECTION_PSP_PS1,0)==&lib.items[2]);assert(library_get_section_app(&lib,SECTION_FAVORITES,0)==&lib.items[2]);assert(library_get_section_app(&lib,SECTION_RECENT,0)==&lib.items[0]);
 assert(library_cache_save(&lib,"media-cache.bin") && library_cache_load(&cached,"media-cache.bin") && cached.count==3 && cached.items[2].installed_at==300 && cached.items[2].media_raw);
 assert(installation_cache_changed("install-state.txt"));assert(installation_cache_commit("install-state.txt") && !installation_cache_changed("install-state.txt"));
 UIContext ui,restored;ui_init(&ui);ui.environment_theme=3;ui.text_theme=2;ui.high_contrast=false;assert(settings_save(&ui,"colors.txt"));ui_init(&restored);settings_load(&restored,"colors.txt");assert(restored.environment_theme==3 && restored.text_theme==2 && !restored.high_contrast);
 theme_configure(1,0,true);assert(theme_background(0xff0d172b)!=0xff0d172b && theme_background(0xff003399)==0xff003399);assert(theme_text(0x80ffffff)==0xfff3f6fc && theme_text(0xff172133)==0xff172133);assert(theme_text_shadow(0xffffffff) && !theme_text_shadow(0xff172133));
 puts("ISO/PBP metadata, PS1 classification, deduplication, newest-install ordering, recent-use ordering, cache persistence, install generation and color/contrast preferences passed.");
}
