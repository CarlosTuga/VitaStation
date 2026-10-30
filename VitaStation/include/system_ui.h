#ifndef VS_SYSTEM_UI_H
#define VS_SYSTEM_UI_H
#include "ui.h"
typedef enum {SYSTEM_UI_LEFT,SYSTEM_UI_RIGHT,SYSTEM_UI_UP,SYSTEM_UI_DOWN,SYSTEM_UI_CONFIRM,SYSTEM_UI_CANCEL} SystemUICommand;
bool system_ui_input(UIContext *ui,Library *library,SystemUICommand command);
bool system_ui_touch(UIContext *ui,Library *library,int x,int y);
bool system_ui_render(UIContext *ui,Library *lib);
void system_ui_update(UIContext *ui,float delta_time);
#endif
