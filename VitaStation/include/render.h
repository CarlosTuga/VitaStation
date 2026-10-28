#ifndef VITA_RENDER_H
#define VITA_RENDER_H

#include "texture.h"
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544

// Colors (ARGB 0xAARRGGBB format)
#define COLOR_BG_DARK        0xFF0D0F12
#define COLOR_PS5_BLUE       0xFF0072CE
#define COLOR_PS5_LIGHT_BLUE 0xFF00A0F0
#define COLOR_WHITE          0xFFFFFFFF
#define COLOR_TEXT_MUTED     0x80FFFFFF
#define COLOR_CARD_BG        0xFF1A222D
#define COLOR_CARD_BORDER    0xFF303C4D
#define COLOR_PANEL_BG       0xCE10151E
#define COLOR_ACCENT_GLOW    0x4000A0F0

bool render_init(void);
void render_shutdown(void);
void render_begin_frame(void);
void render_end_frame(void);
void render_clear(uint32_t color);

void render_fill_rect(int x, int y, int w, int h, uint32_t color);
void render_draw_rect(int x, int y, int w, int h, uint32_t color, int thickness);
void render_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
int render_text_width(float scale,const char *text);
void render_draw_text(int x, int y, uint32_t color, float scale, const char *text);
void render_draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness);
void render_draw_texture_cover(Texture *tex,int x,int y,int w,int h,float alpha);
void render_draw_texture(Texture *tex, int x, int y, int w, int h, float alpha);

#endif // VITA_RENDER_H
