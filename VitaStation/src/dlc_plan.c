#include "dlc_plan.h"
#include <string.h>
DLCPlanResult dlc_plan_run(const CatalogPackage*game,const StoreCatalog*c,DLCInstallCallback install,DLCCancelCallback cancel,void*ctx){
    DLCPlanResult result={0};
    if(!game || !c || c->type!=CATALOG_DLCS || c->status!=CATALOG_READY || !install || game->type!=CATALOG_GAMES || game->vpk)return result;
    for(size_t i=0;i<c->count;i++){
        if(cancel && cancel(ctx)){result.canceled=true;break;}
        const CatalogItem*item=catalog_get(c,i);if(strcmp(item->title_id,game->title_id))continue;
        bool duplicate=false;for(size_t j=0;j<i;j++)if(!strcmp(c->items[j].content_id,item->content_id)){duplicate=true;break;}
        if(duplicate){result.duplicates++;continue;}
        CatalogPackage package;char error[256];
        if(!catalog_get_package(c,i,&package,error,sizeof(error))){result.unavailable++;continue;}
        if(package.type!=CATALOG_DLCS || package.vpk || strcmp(package.title_id,game->title_id)){result.unavailable++;continue;}
        if(install(&package,ctx))result.installed++;
        else if(cancel && cancel(ctx)){result.canceled=true;break;}
        else result.failed++;
    }return result;
}
