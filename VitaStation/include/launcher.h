#ifndef VITA_LAUNCHER_H
#define VITA_LAUNCHER_H

#include <stdbool.h>

typedef enum {
    LAUNCH_ACCEPTED = 0,
    LAUNCH_INVALID_ID,
    LAUNCH_NOT_INSTALLED,
    LAUNCH_PLATFORM_UNAVAILABLE,
    LAUNCH_REJECTED
} LauncherStatus;

typedef struct {
    LauncherStatus status;
    int platform_code;
} LauncherResult;

/* Platform boundary: injected by host regression tests, real adapters on Vita. */
typedef struct {
    bool (*is_installed)(const char *title_id, void *context);
    int (*launch_uri)(int flags, const char *uri, void *context);
    void *context;
} LauncherBackend;

bool launcher_title_id_valid(const char *title_id);
LauncherResult launcher_request_app(const char *title_id, const LauncherBackend *backend);
bool launcher_launch_app(const char *title_id);
LauncherResult launcher_last_result(void);
const char *launcher_error_message(void);

#endif // VITA_LAUNCHER_H
