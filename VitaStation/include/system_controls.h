#ifndef VS_SYSTEM_CONTROLS_H
#define VS_SYSTEM_CONTROLS_H
#include <stddef.h>
typedef enum {SYS_VOLUME,SYS_BRIGHTNESS} SystemControl;
typedef enum {SYS_APPLIED,SYS_APPLIED_UNVERIFIED,SYS_UNAVAILABLE,SYS_READ_FAILED,SYS_WRITE_FAILED,SYS_VERIFY_FAILED,SYS_INVALID,SYS_BRIGHTNESS_SAVED} SystemControlStatus;
typedef struct {SystemControlStatus status;int code,value;} SystemControlResult;
typedef struct {
    int (*read_volume)(int *value);
    int (*write_volume)(int value);
    int (*max_brightness)(int *value);
    int (*write_brightness)(int value);
    int (*read_brightness)(int *value);
    int (*save_brightness)(int value);
} SystemControlBackend;
SystemControlResult system_control_apply(SystemControl control,int value,const SystemControlBackend *backend);
SystemControlResult system_control_set(SystemControl control,int value);
SystemControlResult system_control_read(SystemControl control);
const char *system_control_message(SystemControlStatus status);
typedef enum {SYS_POWER_OFF,SYS_POWER_RESTART} SystemPowerAction;
int system_power_request(SystemPowerAction action);
enum {SYS_ACTION_LIBRARY=200,SYS_ACTION_STORE,SYS_ACTION_PLUGINS,SYS_ACTION_PLUGIN_PREPARE,SYS_ACTION_POWER,SYS_ACTION_AUTOBOOT,SYS_ACTION_PS,SYS_ACTION_PREFERENCES,SYS_ACTION_QUICK,SYS_ACTION_REFRESH,SYS_ACTION_SUSPEND};
typedef struct {const char *category,*name;int control;} SystemFeature;
int system_suspend_request(void);
const char*system_feature_reason(const SystemFeature*feature);
size_t system_feature_count(void);
const SystemFeature *system_feature_get(size_t index);
#endif
