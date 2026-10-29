#include "ui.h"
#include "system_ui.h"
#include "settings_menu.h"
#include "bootstrap.h"
#include "launcher.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool preparing=true,via_shell=false;static int launches;
bool launcher_title_id_valid(const char*id){return id && strlen(id)==9;}
bool bootstrap_start(const Library*l){(void)l;return false;}
unsigned bootstrap_poll(Library*l,int*r,char*t,size_t n){(void)l;(void)r;(void)t;(void)n;return 0;}
bool bootstrap_busy(void){return preparing;}
void bootstrap_shutdown(void){}
bool launcher_launch_app(const char*id){assert(!strcmp(id,"PCSE00001"));launches++;return true;}
bool launcher_exit_required(void){return via_shell;}
const char*launcher_error_message(void){return "test failure";}
int main(){static Library lib;static UIContext u;static TextureCache cache;library_init(&lib);AppItem app={0};strcpy(app.title_id,"PCSE00001");strcpy(app.title,"Test game");app.exists=true;app.type=APP_TYPE_VITA;app.section=SECTION_GAMES;library_add_app(&lib,&app);ui_init(&u);texture_cache_init(&cache,4);
 ui_open_options(&u);assert(u.state==UI_STATE_GAME_SUPPORT);for(int i=0;i<5;i++)ui_navigate_down(&u,&lib);assert(u.game_option==3);ui_render(&u,&lib,&cache);ui_confirm(&u,&lib);assert(u.state==UI_STATE_REMOVE_GAME && u.power_index==0);ui_confirm(&u,&lib);assert(u.state==UI_STATE_GAME_SUPPORT);ui_cancel(&u);assert(u.state==UI_STATE_HOME);
 ui_open_quick_menu(&u);ui_confirm(&u,&lib);assert(u.state==UI_STATE_SETTINGS);ui_cancel(&u);ui_open_quick_menu(&u);for(int i=0;i<8;i++)ui_navigate_right(&u,&lib);assert(u.quick_menu_index==3);ui_confirm(&u,&lib);assert(u.rescan_requested);
 u.state=UI_STATE_SETTINGS;u.settings_group=SETTINGS_VITASTATION;u.settings_index=14;ui_confirm(&u,&lib);assert(u.state==UI_STATE_UNINSTALL && !u.uninstall_restore && u.power_index==0);ui_toggle_favorite(&u,&lib);assert(u.uninstall_restore && !lib.items[0].is_favorite);ui_cancel(&u);assert(u.state==UI_STATE_SETTINGS);
 u.state=UI_STATE_HOME;u.current_section=SECTION_GAMES;u.selected_card_index=0;
 ui_confirm(&u,&lib);assert(launches==1 && !u.pending_launch_id[0] && !u.exit_requested);
 preparing=false;via_shell=true;ui_confirm(&u,&lib);assert(launches==2 && u.exit_requested);
 puts("Installed apps launch during optional preparation; only acknowledged shell handoff requests process exit.");
 puts("Triangle: combined content/cheats/mods actions and protected uninstall confirmation; Start: settings first and refresh last; uninstall: explicit opt-in, cancel default, checkbox does not favorite game.");}
