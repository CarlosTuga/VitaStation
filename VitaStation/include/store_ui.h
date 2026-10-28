#ifndef VS_STORE_UI_H
#define VS_STORE_UI_H
#include "ui.h"
typedef enum {STORE_UI_LEFT,STORE_UI_RIGHT,STORE_UI_UP,STORE_UI_DOWN,STORE_UI_CONFIRM,STORE_UI_CANCEL,STORE_UI_INFO,STORE_UI_REFRESH} StoreUICommand;
bool store_ui_input(UIContext *ui,StoreUICommand command);
bool store_ui_touch(UIContext *ui,int x,int y);
void store_ui_render(UIContext *ui,TextureCache *cache);
void store_ui_progress(void);
#endif
