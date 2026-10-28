#ifndef VITA_LIBRARY_H
#define VITA_LIBRARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_APPS 256
#define MAX_PATH_LEN 256
#define MAX_TITLE_LEN 128
#define MAX_TITLE_ID_LEN 32

typedef enum {
    SECTION_GAMES = 0,
    SECTION_MEDIA = 1, /* compatibility alias: homebrew */
    SECTION_HOMEBREW = 1, SECTION_RECENT = 2, SECTION_FAVORITES = 3, SECTION_STORE = 4, SECTION_PSP_PS1 = 5, SECTION_COUNT = 6
} AppSection;

typedef enum {
    APP_TYPE_VITA = 0,
    APP_TYPE_PSP = 1,
    APP_TYPE_PS1 = 2,
    APP_TYPE_HOMEBREW = 3,
    APP_TYPE_SYSTEM = 4,
    APP_TYPE_MEDIA = 5, APP_TYPE_UNKNOWN = 6
} AppType;

typedef struct {
    char title_id[MAX_TITLE_ID_LEN];
    char title[MAX_TITLE_LEN];
    char category_str[32];
    char content_id[64];
    bool classification_override;
    AppType type;
    AppSection section;
    char icon_path[MAX_PATH_LEN];
    char pic_path[MAX_PATH_LEN];
    char media_path[MAX_PATH_LEN];
    char game_id[16];
    bool media_raw;
    bool is_favorite;
    uint64_t last_played;
    uint64_t installed_at;
    int launch_count;
    bool exists;
} AppItem;

typedef struct {
    bool scan_complete;
    AppItem items[MAX_APPS];
    int count;
    int games_indices[MAX_APPS];
    int games_count;
    int media_indices[MAX_APPS];
    int media_count;
} Library;

uint64_t library_date(int year,int month,int day,int hour,int minute,int second);
uint64_t library_file_date(const char*path);
int library_section_count(Library *lib, AppSection section);
void library_init(Library *lib);
int library_scan(Library *lib, const char *root_path);
int library_parse_param_sfo(const char *sfo_path, char *out_title, size_t title_size, char *out_category, size_t cat_size);
bool library_add_app(Library *lib, const AppItem *app);
AppItem* library_get_app(Library *lib, int global_index);
AppItem* library_get_section_app(Library *lib, AppSection section, int section_index);
void library_update_indices(Library *lib);
/* Catalog-backed classification; no cryptographic claim of publisher authenticity. */
void library_classify_catalog(Library *lib,const char *games_path,const char *demos_path);
void library_toggle_favorite(Library *lib, const char *title_id);
void library_record_launch(Library *lib, const char *title_id);

#endif // VITA_LIBRARY_H
