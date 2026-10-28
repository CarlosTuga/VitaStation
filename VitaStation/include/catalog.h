#ifndef VS_CATALOG_H
#define VS_CATALOG_H
#ifndef VS_STORE_COVERS_DIR
#define VS_STORE_COVERS_DIR "ux0:data/VitaStation/covers"
#endif
#ifndef VS_PACKED_COVERS_DIR
#define VS_PACKED_COVERS_DIR "app0:/assets/store-covers"
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CATALOG_GAMES=0,CATALOG_DLCS,CATALOG_THEMES,CATALOG_DEMOS,
    CATALOG_PSP,CATALOG_PSP_DLCS,CATALOG_PS1,CATALOG_PSM,CATALOG_HOMEBREW,CATALOG_TYPE_COUNT
} CatalogType;
typedef enum {CATALOG_IDLE,CATALOG_READY,CATALOG_MISSING,CATALOG_INVALID,CATALOG_IO_ERROR} CatalogStatus;
typedef enum {CATALOG_LINK_VPK,CATALOG_LINK_APP_ZIP,CATALOG_LINK_DATA_ZIP,CATALOG_LINK_DATA_FILE,CATALOG_LINK_PLUGIN,CATALOG_LINK_REFERENCE} CatalogLinkFormat;
typedef struct {
    CatalogLinkFormat format;
    char name[96],url[1024],extract_path[256],section[32],filename[96];
    uint64_t size_bytes;
    bool recommended,required;
} CatalogLink;
typedef struct {
    char id[96],description[1024],requirements[1024],author[128],version[32],category[64],icon_url[1024];
} HomebrewMetadata;
typedef struct {
    char title_id[10],name[192],region[16],content_id[64],cover[256];
    uint64_t size_bytes;
    bool has_size;
    long source_offset;
    bool cover_resolved;
    unsigned cover_revision;
    char cache_key[64];
    HomebrewMetadata *homebrew;
} CatalogItem;
typedef struct {
    CatalogItem *items;
    size_t count,capacity,skipped;
    CatalogType type;
    CatalogStatus status;
    bool truncated;
    bool source_json;
    char root[256],cover_dir[256],source_path[256];
    char title_filter[10];
    unsigned cover_revision;
} StoreCatalog;

typedef struct {
    CatalogType type;
    char title_id[10], name[192], content_id[64], url[1024], zrif[2048], digest[65];
    uint64_t size_bytes;
    bool vpk;
    bool auto_dlcs;
    CatalogLinkFormat format;
    char cache_key[64],extract_path[256],plugin_section[32],filename[96];
} CatalogPackage;
bool catalog_get_package(const StoreCatalog *catalog,size_t index,CatalogPackage *package,char *error,size_t error_size);
size_t catalog_homebrew_links(const StoreCatalog *catalog,size_t index,CatalogLink *links,size_t capacity);
bool catalog_homebrew_package(const StoreCatalog *catalog,size_t index,const CatalogLink *link,CatalogPackage *package,char *error,size_t error_size);

void catalog_init(StoreCatalog *catalog);
void catalog_clear(StoreCatalog *catalog);
const char *catalog_type_name(CatalogType type);
bool catalog_load_root(StoreCatalog *catalog,const char *root,CatalogType type);
bool catalog_load_auto(StoreCatalog *catalog,CatalogType type);
bool catalog_load_matching_root(StoreCatalog *catalog,const char *root,CatalogType type,const char *title_id);
bool catalog_load_matching_auto(StoreCatalog *catalog,CatalogType type,const char *title_id);
const CatalogItem *catalog_get(const StoreCatalog *catalog,size_t index);
const char *catalog_resolve_cover(StoreCatalog *catalog,size_t index);
void catalog_invalidate_covers(StoreCatalog *catalog);
#ifdef __cplusplus
}
#endif
#endif
