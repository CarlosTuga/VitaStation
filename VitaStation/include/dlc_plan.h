#ifndef VS_DLC_PLAN_H
#define VS_DLC_PLAN_H
#include "catalog.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {size_t installed,failed,unavailable,duplicates;bool canceled;} DLCPlanResult;
typedef bool (*DLCInstallCallback)(const CatalogPackage *package,void *context);
typedef bool (*DLCCancelCallback)(void *context);
DLCPlanResult dlc_plan_run(const CatalogPackage *game,const StoreCatalog *catalog,DLCInstallCallback install,DLCCancelCallback cancel,void *context);
#ifdef __cplusplus
}
#endif
#endif
