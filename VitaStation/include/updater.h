#ifndef VS_UPDATER_H
#define VS_UPDATER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {int phase;char message[256],version[32],url[1024],digest[65];unsigned long size;uint64_t done,total;} UpdateStatus;
/* phase: 0 idle, 1 checking/downloading, 2 available, 3 ready to install, 4 current, 5 error, 6 handoff/install */
bool update_select_covers(const char *json,size_t length,UpdateStatus *result);
bool update_select(const char *json,size_t length,UpdateStatus *result);
bool store_update_install(char *error,size_t capacity);
void store_update_fail(const char *message);
void store_update_cleanup(void);
void store_update_request(void);
void store_update_status(UpdateStatus *status);
#ifdef __cplusplus
}
#endif
#endif
