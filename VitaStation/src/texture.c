#include "texture.h"
#include "cover_pack.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__vita__)
#include <vita2d.h>
#endif

void texture_cache_init(TextureCache *cache, uint32_t max_mem_mb) {
    if (!cache) return;
    memset(cache, 0, sizeof(TextureCache));
    cache->max_memory_bytes = (max_mem_mb == 0 ? 32 : max_mem_mb>128?128:max_mem_mb) * 1024 * 1024;
}

void texture_cache_evict_lru(TextureCache *cache) {
    if (!cache) return;
    int oldest_idx = -1;
    uint64_t min_access = UINT64_MAX;

    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (cache->slots[i].in_use && !cache->slots[i].pins && cache->slots[i].last_access < min_access) {
            min_access = cache->slots[i].last_access;
            oldest_idx = i;
        }
    }

    if (oldest_idx >= 0) {
        Texture *t = &cache->slots[oldest_idx];
#if defined(__vita__)
        if (t->vita_tex) {
            vita2d_wait_rendering_done();   /* o GPU pode ainda estar a usar a textura */
            vita2d_free_texture((vita2d_texture*)t->vita_tex);
            t->vita_tex = NULL;
        }
#endif
        if (t->pixels) {
            free(t->pixels);
            t->pixels = NULL;
        }
        uint32_t freed_bytes = t->width * t->height * 4;
        if (cache->current_memory_bytes >= freed_bytes) {
            cache->current_memory_bytes -= freed_bytes;
        } else {
            cache->current_memory_bytes = 0;
        }
        t->in_use = false;
    }
}

Texture* texture_create_placeholder(TextureCache *cache, const char *key, uint32_t width, uint32_t height, uint32_t color) {
    if (!cache || !key || strlen(key)>=MAX_TEX_PATH || !width || !height || width>1920 || height>1088 || (uint64_t)width*height*4>cache->max_memory_bytes) return NULL;

    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (cache->slots[i].in_use && strcmp(cache->slots[i].path, key) == 0) {
            cache->slots[i].last_access = ++cache->access_counter;
            return &cache->slots[i];
        }
    }

    uint32_t size_bytes = width * height * 4;

    // Evict until memory available or no more items can be evicted
    int evictions = 0;
    while (cache->current_memory_bytes + size_bytes > cache->max_memory_bytes && evictions < TEXTURE_CACHE_SIZE) {
        texture_cache_evict_lru(cache);
        evictions++;
    }

    if ((uint64_t)cache->current_memory_bytes + size_bytes > cache->max_memory_bytes) return NULL;
    int free_slot = -1;
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (!cache->slots[i].in_use) {
            free_slot = i;
            break;
        }
    }

    if (free_slot < 0) {
        texture_cache_evict_lru(cache);
        for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
            if (!cache->slots[i].in_use) {
                free_slot = i;
                break;
            }
        }

    }

    if (free_slot < 0) return NULL;

    Texture *t = &cache->slots[free_slot];
    memset(t, 0, sizeof(Texture));
    snprintf(t->path, sizeof(t->path) - 1, "%s", key);
    t->width = width;
    t->height = height;
    t->pixels = (uint32_t*)malloc(size_bytes);
    if (!t->pixels) {
        return NULL;
    }
    for (size_t i = 0; i < width * height; i++) {
        t->pixels[i] = color;
    }
    t->in_use = true;
    t->last_access = ++cache->access_counter;
    cache->current_memory_bytes += size_bytes;

    return t;
}

#if defined(__vita__)
/* Le so o cabecalho PNG (assinatura + IHDR) e recusa imagens enormes ANTES de as decodificar. */
#define TEX_MAX_W 1920u
#define TEX_MAX_H 1088u
static bool png_dimensions_ok(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    unsigned char h[24];
    size_t n = fread(h, 1, sizeof(h), f);
    fclose(f);
    if (n < sizeof(h) || memcmp(h, "\x89PNG\r\n\x1a\n", 8) != 0 || memcmp(h+12,"IHDR",4)!=0 || h[8]!=0 || h[9]!=0 || h[10]!=0 || h[11]!=13) return false;
    uint32_t w  = ((uint32_t)h[16] << 24) | ((uint32_t)h[17] << 16) | ((uint32_t)h[18] << 8) | h[19];
    uint32_t ht = ((uint32_t)h[20] << 24) | ((uint32_t)h[21] << 16) | ((uint32_t)h[22] << 8) | h[23];
    return w > 0 && ht > 0 && w <= TEX_MAX_W && ht <= TEX_MAX_H;
}
/* Usagi also decodes PNG covers from memory on Vita. Keep the size check above. */
static vita2d_texture* load_png_memory(const char*path){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long size=ftell(f);
    if(size<=0 || size>8*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    unsigned char*bytes=malloc((size_t)size);if(!bytes){fclose(f);return NULL;}
    bool complete=fread(bytes,1,(size_t)size,f)==(size_t)size;fclose(f);
    vita2d_texture*t=complete?vita2d_load_PNG_buffer(bytes):NULL;free(bytes);return t;
}
static bool packed_dimensions_ok(const unsigned char*b,size_t n){
    if(n>=24 && !memcmp(b,"\x89PNG\r\n\x1a\n",8)){
        uint32_t w=((uint32_t)b[16]<<24)|((uint32_t)b[17]<<16)|(b[18]<<8)|b[19];
        uint32_t h=((uint32_t)b[20]<<24)|((uint32_t)b[21]<<16)|(b[22]<<8)|b[23];
        return !memcmp(b+12,"IHDR",4) && w && h && w<=TEX_MAX_W && h<=TEX_MAX_H;
    }
    if(n<4 || b[0]!=255 || b[1]!=216)return false;
    size_t at=2;while(at+4<=n){
        if(b[at++]!=255)return false;
        unsigned m=b[at++];while(m==255 && at<n)m=b[at++];
        if(m==0xda || m==0xd9)return false;
        if(m==1 || (m>=0xd0 && m<=0xd7))continue;
        if(at+2>n)return false;
        unsigned len=(b[at]<<8)|b[at+1];if(len<2 || len>n-at)return false;
        if(m>=0xc0 && m<=0xcf && m!=0xc4 && m!=0xc8 && m!=0xcc){
            if(len<7)return false;
            unsigned h=(b[at+3]<<8)|b[at+4],w=(b[at+5]<<8)|b[at+6];return w && h && w<=TEX_MAX_W && h<=TEX_MAX_H;
        }at+=len;
    }return false;
}
static bool jpeg_dimensions_ok(const char *path){
    FILE*f=fopen(path,"rb");if(!f)return false;
    bool ok=false;
    if(fgetc(f)==0xff && fgetc(f)==0xd8){
        for(unsigned i=0;i<2048;i++){
            int prefix=fgetc(f),marker=fgetc(f);if(prefix!=0xff || marker==EOF)break;
            while(marker==0xff)marker=fgetc(f);
            if(marker==0xda || marker==0xd9 || marker==EOF)break;
            if(marker==0x01 || (marker>=0xd0 && marker<=0xd7))continue;
            int hi=fgetc(f),lo=fgetc(f);if(hi==EOF || lo==EOF)break;int length=(hi<<8)|lo;if(length<2)break;
            if((marker>=0xc0 && marker<=0xcf) && marker!=0xc4 && marker!=0xc8 && marker!=0xcc){
                unsigned char b[5];if(length<7 || fread(b,1,5,f)!=5)break;
                unsigned h=(b[1]<<8)|b[2],w=(b[3]<<8)|b[4];ok=w>0 && h>0 && w<=TEX_MAX_W && h<=TEX_MAX_H;break;
            }
            if(fseek(f,length-2,SEEK_CUR))break;
        }
    }
    fclose(f);return ok;
}
#endif

Texture* texture_cache_load(TextureCache *cache, const char *path) {
    if (!cache || !path || !*path || strlen(path)>=MAX_TEX_PATH) return NULL;
    for(int i=0;i<16;i++)if(!strcmp(path,cache->failed_paths[i]))return NULL;

    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (cache->slots[i].in_use && strcmp(cache->slots[i].path, path) == 0) {
            cache->slots[i].last_access = ++cache->access_counter;
            return &cache->slots[i];
        }
    }

#if defined(__vita__)
    vita2d_texture *vt=NULL;
    if(!strncmp(path,"vspack:",7)){
        size_t n=0;unsigned char*b=cover_pack_read(path,&n);
        if(b && packed_dimensions_ok(b,n)){
            /* Bundled, CRC-checked images were bounded to 224x320 during packaging. */
            if(n>=24 && !memcmp(b,"\x89PNG\r\n\x1a\n",8))vt=vita2d_load_PNG_buffer(b);
            else if(n>=3 && b[0]==255 && b[1]==216 && b[2]==255)vt=vita2d_load_JPEG_buffer(b,n);
        }
        free(b);
    }else vt=png_dimensions_ok(path) ? load_png_memory(path) : jpeg_dimensions_ok(path)?vita2d_load_JPEG_file(path):NULL;
    if (vt) {
        uint32_t w = vita2d_texture_get_width(vt);
        uint32_t h = vita2d_texture_get_height(vt);
        uint32_t size_bytes = w * h * 4;

        int evictions = 0;
        while (cache->current_memory_bytes + size_bytes > cache->max_memory_bytes && evictions < TEXTURE_CACHE_SIZE) {
            texture_cache_evict_lru(cache);
            evictions++;
        }

        if ((uint64_t)cache->current_memory_bytes + size_bytes > cache->max_memory_bytes) {vita2d_free_texture(vt);return NULL;}
    int free_slot = -1;
        for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
            if (!cache->slots[i].in_use) {
                free_slot = i;
                break;
            }
        }

        if(free_slot<0){
            texture_cache_evict_lru(cache);
            for(int i=0;i<TEXTURE_CACHE_SIZE;i++)if(!cache->slots[i].in_use){free_slot=i;break;}
        }
        if (free_slot >= 0) {
            Texture *t = &cache->slots[free_slot];
            memset(t, 0, sizeof(Texture));
            snprintf(t->path, sizeof(t->path) - 1, "%s", path);
            t->width = w;
            t->height = h;
            t->vita_tex = vt;
            t->in_use = true;
            t->last_access = ++cache->access_counter;
            cache->current_memory_bytes += size_bytes;
            return t;
        } else {
            vita2d_free_texture(vt);
        }
    }
#endif

    snprintf(cache->failed_paths[cache->failed_next++%16],MAX_TEX_PATH,"%s",path);
    char message[320];snprintf(message,sizeof(message),"artwork unavailable or invalid: %s",path);
    vs_log("texture",message,-1);
    return NULL; /* Missing artwork uses procedural UI background. */
}

void texture_cache_retry(TextureCache *cache){if(cache){memset(cache->failed_paths,0,sizeof(cache->failed_paths));cache->failed_next=0;}}

void texture_cache_clear(TextureCache *cache) {
    if (!cache) return;
#if defined(__vita__)
    vita2d_wait_rendering_done();           /* uma unica espera antes de libertar tudo */
#endif
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (cache->slots[i].in_use) {
#if defined(__vita__)
            if (cache->slots[i].vita_tex) {
                vita2d_free_texture((vita2d_texture*)cache->slots[i].vita_tex);
                cache->slots[i].vita_tex = NULL;
            }
#endif
            if (cache->slots[i].pixels) {
                free(cache->slots[i].pixels);
                cache->slots[i].pixels = NULL;
            }
            cache->slots[i].in_use = false;
        }
    }
    cache->current_memory_bytes = 0;
}
