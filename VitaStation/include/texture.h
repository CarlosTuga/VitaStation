#ifndef VITA_TEXTURE_H
#define VITA_TEXTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TEXTURE_CACHE_SIZE 64
#define MAX_TEX_PATH 256

typedef struct {
    char path[MAX_TEX_PATH];
    uint32_t width;
    uint32_t height;
    uint32_t *pixels; // ARGB8888 software buffer for rendering/host test
    uint64_t last_access;
    unsigned pins;
    bool in_use;
    void *vita_tex;   // Vita Vita2d/GXM texture handle
} Texture;

typedef struct {
    char failed_paths[16][MAX_TEX_PATH];
    unsigned failed_next;
    Texture slots[TEXTURE_CACHE_SIZE];
    uint32_t max_memory_bytes;
    uint32_t current_memory_bytes;
    uint64_t access_counter;
} TextureCache;

void texture_cache_init(TextureCache *cache, uint32_t max_mem_mb);
Texture* texture_cache_load(TextureCache *cache, const char *path);
Texture* texture_create_placeholder(TextureCache *cache, const char *key, uint32_t width, uint32_t height, uint32_t color);
void texture_cache_evict_lru(TextureCache *cache);
void texture_cache_retry(TextureCache *cache);
void texture_cache_clear(TextureCache *cache);

#endif // VITA_TEXTURE_H
