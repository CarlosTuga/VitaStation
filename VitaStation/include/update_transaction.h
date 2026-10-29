#ifndef VS_UPDATE_TRANSACTION_H
#define VS_UPDATE_TRANSACTION_H
#include <stdbool.h>
typedef enum {UPDATE_OK,UPDATE_INVALID,UPDATE_INSTALLER_ERROR,UPDATE_RECOVERED,UPDATE_RECOVERY_PENDING,UPDATE_RESTART_FAILED,UPDATE_INSTALL_FAILED} UpdateOutcome;
typedef struct {
 void *context;
 bool (*verify)(void*,bool rollback);
 int (*initialize)(void*);
 int (*promote)(void*,bool rollback);
 int (*restart)(void*);
 bool rollback_enabled;
} UpdateBackend;
UpdateOutcome update_transaction(const UpdateBackend *backend,int *code);
#endif
