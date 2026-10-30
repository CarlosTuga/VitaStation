#ifndef VS_THEME_H
#define VS_THEME_H
#include <stdint.h>
#include <stdbool.h>
#define VS_THEME_COUNT 15
uint32_t theme_canvas_color(int environment,int y);
void theme_configure(int environment,int text,bool contrast);
uint32_t theme_background(uint32_t color);
uint32_t theme_text(uint32_t color);
bool theme_text_shadow(uint32_t color);
const char*theme_environment_name(int index);
const char*theme_text_name(int index);
#endif
