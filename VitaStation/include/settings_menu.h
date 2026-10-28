#ifndef VS_SETTINGS_MENU_H
#define VS_SETTINGS_MENU_H
#include "ui.h"
#include "system_ui.h"
enum {SETTINGS_ROOT=0,SETTINGS_CONSOLE=1,SETTINGS_VITASTATION=2};
int settings_group_for(int raw_index);
int settings_group_count(int group);
int settings_group_item(int group,int position);
int settings_group_position(int group,int raw_index);
void settings_menu_open(UIContext*ui);
bool settings_menu_input(UIContext*ui,SystemUICommand command);
bool settings_menu_touch(UIContext*ui,Library*lib,int x,int y);
void settings_menu_icon(int x,int y,int size,int group,uint32_t color);
void settings_option_icon(int x,int y,int size,int raw_index,uint32_t color);
void settings_menu_root(UIContext*ui);
#endif
