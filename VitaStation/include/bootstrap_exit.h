#ifndef VS_BOOTSTRAP_EXIT_H
#define VS_BOOTSTRAP_EXIT_H
#include <stdbool.h>
typedef enum {VS_BOOT_SCAN,VS_BOOT_PREPARE,VS_BOOT_DONE} BootstrapPhase;
typedef struct {BootstrapPhase phase;bool exiting;} BootstrapExit;
/* Caller serializes these transitions with the startup mutex. */
static inline bool bootstrap_exit_request(BootstrapExit*s){s->exiting=true;return s->phase==VS_BOOT_PREPARE;}
static inline bool bootstrap_prepare_begin(BootstrapExit*s){if(s->exiting)return false;s->phase=VS_BOOT_PREPARE;return true;}
static inline void bootstrap_work_done(BootstrapExit*s){s->phase=VS_BOOT_DONE;}
bool bootstrap_record_components(const char*marker);
#endif
