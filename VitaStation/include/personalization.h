#ifndef VS_PERSONALIZATION_H
#define VS_PERSONALIZATION_H
#include "system_ui.h"
bool personalization_input(UIContext*u,Library*lib,SystemUICommand command);
bool personalization_touch(UIContext*u,Library*lib,int x,int y);
bool personalization_render(UIContext*u,TextureCache*cache);
void personalization_background(UIContext*u,TextureCache*cache);
bool personalization_image_valid(const char*path,bool lockscreen);
#endif
