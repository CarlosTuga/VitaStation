#include "integrations.h"
#include "library_cache.h"
#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <ctype.h>

#if defined(__vita__)

#include <psp2/io/dirent.h>

#endif

size_t system_apps_scan(AppItem*out,size_t capacity){size_t count=0;

#if defined(__vita__)

 /* Only the user-facing apps from the former console-applications entry.
    Avoid similarly named internal services (PhotoExport/background browser). */
 const char*ids[]={"NPXS10009","NPXS10010","NPXS10004","NPXS10008","NPXS10003","NPXS10026"};
 for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]) && count<capacity;i++){
  const char*id=ids[i];char path[256],title[128],category[32];snprintf(path,sizeof(path),"vs0:app/%s/sce_sys/param.sfo",id);
  if(library_parse_param_sfo(path,title,sizeof(title),category,sizeof(category))<0)continue;
  AppItem*app=&out[count++];memset(app,0,sizeof(*app));snprintf(app->title_id,sizeof(app->title_id),"%s",id);snprintf(app->title,sizeof(app->title),"%s",title);app->type=APP_TYPE_SYSTEM;app->section=SECTION_VITA;app->exists=true;app->installed_at=library_file_date(path);
  snprintf(app->icon_path,sizeof(app->icon_path),"vs0:app/%s/sce_sys/icon0.png",id);snprintf(app->pic_path,sizeof(app->pic_path),"vs0:app/%s/sce_sys/pic0.png",id);
 }

#else

 (void)out;(void)capacity;

#endif

 return count;

}


/* A small separate cache avoids scanning system SFOs on every launcher start.
   AppItem/cache wire layout is unchanged, so the existing library stays usable. */
void system_apps_populate(Library*lib,const char*cache_path){
 if(!lib || !cache_path)return;
 Library*system=calloc(1,sizeof(*system));if(!system)return;
 if(!library_cache_load(system,cache_path)){
  system->count=(int)system_apps_scan(system->items,16);
  if(system->count)library_cache_save(system,cache_path);
 }
 for(int i=0;i<system->count;i++){
  AppItem app=system->items[i];if(app.type!=APP_TYPE_SYSTEM || app.section!=SECTION_VITA)continue;
  for(int j=0;j<lib->count;j++)if(!strcmp(lib->items[j].title_id,app.title_id)){app.is_favorite=lib->items[j].is_favorite;app.last_played=lib->items[j].last_played;app.launch_count=lib->items[j].launch_count;break;}
  library_add_app(lib,&app);
 }
 free(system);
}
