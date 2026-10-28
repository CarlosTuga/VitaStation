#ifndef VS_UPDATER_H
#define VS_UPDATER_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {int phase;char message[256],version[32],url[1024],digest[65];unsigned long size;} UpdateStatus;
/* phase: 0 idle, 1 checking/downloading, 2 available, 3 downloaded, 4 current, 5 error */
bool update_select(const char *json,size_t length,UpdateStatus *result);
void store_update_request(void);
void store_update_status(UpdateStatus *status);
#ifdef __cplusplus
}
#endif
#endif
