#include "render.h"
#include "theme.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if defined(VS_RENDER_TRACE)
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...) ((void)0)
#endif

#if defined(__vita__)
#include <vita2d.h>
static vita2d_pgf *g_pgf = NULL;
static vita2d_font *g_font;
#endif

/* Textures already referenced by this frame must survive until GPU completion. */
static Texture *frame_textures[TEXTURE_CACHE_SIZE];
static unsigned frame_texture_count;
bool render_init(void) {
#if defined(__vita__)
    if(vita2d_init()<0){vs_log("render","vita2d initialization failed",-1);return false;}
    vita2d_set_clear_color(RGBA8(13, 15, 18, 255));
    if (!g_pgf) {
        g_pgf = vita2d_load_default_pgf();
    }
#endif
    return true;
}

bool render_set_font(const char*path){
#if defined(__vita__)
 vita2d_font*next=NULL;if(path && *path){FILE*f=fopen(path,"rb");if(!f)return false;bool valid=!fseek(f,0,SEEK_END) && ftell(f)>0 && ftell(f)<=6*1024*1024;fclose(f);if(!valid)return false;next=vita2d_load_font_file(path);if(!next)return false;}
 vita2d_wait_rendering_done();if(g_font)vita2d_free_font(g_font);g_font=next;return true;
#else
 (void)path;return true;
#endif
}
void render_shutdown(void) {
#if defined(__vita__)
    if(g_font){vita2d_free_font(g_font);g_font=NULL;}
    if (g_pgf) {
        vita2d_free_pgf(g_pgf);
        g_pgf = NULL;
    }
    vita2d_fini();
#endif
}

void render_begin_frame(void) {
    TRACE("B\n");
    frame_texture_count=0;
#if defined(__vita__)
    vita2d_start_drawing();
    vita2d_clear_screen();
#endif
}

void render_end_frame(void) {
    TRACE("E\n");
#if defined(__vita__)
    vita2d_end_drawing();
    vita2d_swap_buffers();
    vita2d_wait_rendering_done();
#endif
    for(unsigned i=0;i<frame_texture_count;i++)if(frame_textures[i]->pins)frame_textures[i]->pins--;
    frame_texture_count=0;
}

void render_clear(uint32_t color) {
#if defined(__vita__)
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    vita2d_set_clear_color(RGBA8(r, g, b, a));
#else
    (void)color;
#endif
}

void render_fill_rect(int x, int y, int w, int h, uint32_t color) {
    color=theme_background(color);
    TRACE("R\t%d\t%d\t%d\t%d\t%08x\n",x,y,w,h,(unsigned)color);
#if defined(__vita__)
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    vita2d_draw_rectangle((float)x, (float)y, (float)w, (float)h, RGBA8(r, g, b, a));
#else
    (void)x; (void)y; (void)w; (void)h; (void)color;
#endif
}

void render_draw_rect(int x, int y, int w, int h, uint32_t color, int thickness) {
    if (thickness <= 0) return;
    render_fill_rect(x, y, w, thickness, color);
    render_fill_rect(x, y + h - thickness, w, thickness, color);
    render_fill_rect(x, y, thickness, h, color);
    render_fill_rect(x + w - thickness, y, thickness, h, color);
}

void render_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius <= 1) {
        render_fill_rect(x, y, w, h, color);
        return;
    }
    /* corpo central + cantos aproximados por linhas horizontais (sem sobreposicao) */
    render_fill_rect(x, y + radius, w, h - 2 * radius, color);
    for (int i = 0; i < radius; i++) {
        float dy = (float)radius - (float)i - 0.5f;
        int inset = radius - (int)(sqrtf((float)(radius * radius) - dy * dy) + 0.5f);
        render_fill_rect(x + inset, y + i, w - 2 * inset, 1, color);
        render_fill_rect(x + inset, y + h - 1 - i, w - 2 * inset, 1, color);
    }
}

int render_text_width(float scale,const char *text){
    if(!text)return 0;
#if defined(__vita__)
    if(g_font)return vita2d_font_text_width(g_font,(unsigned)(24*scale),text);
    return g_pgf?vita2d_pgf_text_width(g_pgf,scale,text):0;
#else
    return (int)(strlen(text)*10*scale);
#endif
}
void render_draw_text(int x, int y, uint32_t color, float scale, const char *text) {
    if (!text || text[0] == '\0') return;
    color=theme_text(color);
    TRACE("T\t%d\t%d\t%08x\t%f\t%s\n",x,y,(unsigned)color,scale,text);
#if defined(__vita__)
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    if(g_font){unsigned size=(unsigned)(24*scale);if(size<8)size=8;if(theme_text_shadow(color))vita2d_font_draw_text(g_font,x+1,y+1,RGBA8(0,0,0,220),size,text);vita2d_font_draw_text(g_font,x,y,RGBA8(r,g,b,a),size,text);}
    else if (g_pgf) {
        if(theme_text_shadow(color))vita2d_pgf_draw_text(g_pgf,x+1,y+1,RGBA8(0,0,0,220),scale,text);
        vita2d_pgf_draw_text(g_pgf, x, y, RGBA8(r, g, b, a), scale, text);
    }
#else
    (void)x; (void)y; (void)color; (void)scale;
#endif
}

void render_draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness) {
    TRACE("L\t%d\t%d\t%d\t%d\t%08x\t%d\n",x1,y1,x2,y2,(unsigned)color,thickness);
    (void)thickness;
#if defined(__vita__)
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    vita2d_draw_line((float)x1, (float)y1, (float)x2, (float)y2, RGBA8(r, g, b, a));
#else
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
#endif
}

static bool frame_hold_texture(Texture *tex){
    if (!tex || !tex->in_use) return false;
    bool tracked=false;
    for(unsigned i=0;i<frame_texture_count;i++)if(frame_textures[i]==tex)tracked=true;
    if(!tracked){if(frame_texture_count==TEXTURE_CACHE_SIZE)return false;frame_textures[frame_texture_count++]=tex;tex->pins++;}
    return true;
}
void render_draw_texture_cover(Texture *tex,int x,int y,int w,int h,float alpha){
    if(w<=0 || h<=0 || !tex || !tex->width || !tex->height || !frame_hold_texture(tex))return;
#if defined(__vita__)
    if(tex->vita_tex){
        float scale=fmaxf((float)w/tex->width,(float)h/tex->height);
        float sw=w/scale,sh=h/scale;
        vita2d_draw_texture_tint_part_scale((vita2d_texture*)tex->vita_tex,x,y,(tex->width-sw)*.5f,(tex->height-sh)*.5f,sw,sh,scale,scale,RGBA8(255,255,255,(uint8_t)(alpha*255)));
    }
#else
    (void)x;(void)y;(void)alpha;
#endif
}
void render_draw_texture(Texture *tex, int x, int y, int w, int h, float alpha) {
    if(!frame_hold_texture(tex))return;
#if defined(__vita__)
    if (tex->vita_tex) {
        float x_scale = (float)w / (float)tex->width;
        float y_scale = (float)h / (float)tex->height;
        uint8_t a = (uint8_t)(alpha * 255.0f);
        vita2d_draw_texture_tint_scale((vita2d_texture*)tex->vita_tex, (float)x, (float)y, x_scale, y_scale, RGBA8(255, 255, 255, a));
    }
#else
    (void)x; (void)y; (void)w; (void)h; (void)alpha;
#endif
}
