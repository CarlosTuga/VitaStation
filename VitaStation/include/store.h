#ifndef VS_STORE_H
#define VS_STORE_H
#include "catalog.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {STORE_IDLE,STORE_DOWNLOADING,STORE_INSTALLING,STORE_COMPLETE,STORE_FAILED,STORE_CANCELED,STORE_SYSTEM_QUEUED} StorePhase;
typedef struct {
    StorePhase phase;
    unsigned queued,completed,failed,revision;
    unsigned system_submitted;
    unsigned covers_revision;
    unsigned covers_failed;
    char cover_error[128];
    unsigned homebrew_revision;
    char homebrew_error[128];
    uint64_t downloaded,total;
    char title[192],message[256];
    bool active;
    bool offline_covers;
} StoreProgress;
bool store_start(void);
bool store_enqueue(const CatalogPackage *package,char *error,size_t size);
void store_progress(StoreProgress *progress);
void store_cancel(void);
bool store_busy(void);
void store_request_cover(CatalogType type,const char *title_id);
void store_retry_covers(void);
void store_request_image(CatalogType type,const char *key,const char *url);
void store_refresh_homebrew(bool force);
void store_shutdown(void);
#ifdef __cplusplus
}
#endif
#endif
