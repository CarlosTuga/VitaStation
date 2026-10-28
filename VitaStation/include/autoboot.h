#ifndef VS_AUTOBOOT_H
#define VS_AUTOBOOT_H
#include <stdbool.h>
bool autoboot_is_enabled(void);
bool autoboot_set(bool enabled);
bool autoboot_bridge_available(void);
/* Marker persistence tested independently of the shell plugin and hardware. */
bool autoboot_marker_write(const char *path,bool enabled);
bool autoboot_marker_read(const char *path);
#endif
