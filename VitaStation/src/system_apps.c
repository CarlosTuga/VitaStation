#include "integrations.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(__vita__)
#include <psp2/io/dirent.h>
#endif
size_t system_apps_scan(AppItem*out,size_t capacity){size_t count=0;
#if defined(__vita__)
 SceUID dir=sceIoDopen("vs0:app");if(dir<0)return 0;
 while(count<capacity){SceIoDirent entry;memset(&entry,0,sizeof(entry));if(sceIoDread(dir,&entry)<=0)break;
  if(strlen(entry.d_name)!=9 || strncmp(entry.d_name,"NPXS",4))continue;
  char path[256],title[128],category[32];snprintf(path,sizeof(path),"vs0:app/%.9s/sce_sys/param.sfo",entry.d_name);
  if(library_parse_param_sfo(path,title,sizeof(title),category,sizeof(category))<0)continue;
  char lower[128];for(size_t i=0;i<sizeof(lower);i++){lower[i]=(char)tolower((unsigned char)title[i]);if(!title[i])break;}lower[127]=0;
  const char*words[]={"music","música","video","vídeo","photo","troph","browser","content","cma","fotograf","trof","naveg","conte"};bool wanted=false;for(unsigned i=0;i<sizeof(words)/sizeof(words[0]);i++)if(strstr(lower,words[i]))wanted=true;if(!wanted)continue;
  AppItem*app=&out[count++];memset(app,0,sizeof(*app));snprintf(app->title_id,sizeof(app->title_id),"%.9s",entry.d_name);snprintf(app->title,sizeof(app->title),"%s",title);app->type=APP_TYPE_SYSTEM;app->exists=true;snprintf(app->icon_path,sizeof(app->icon_path),"vs0:app/%.9s/sce_sys/icon0.png",entry.d_name);snprintf(app->pic_path,sizeof(app->pic_path),"vs0:app/%.9s/sce_sys/pic0.png",entry.d_name);
 }sceIoDclose(dir);
#else
 (void)out;(void)capacity;
#endif
 return count;
}
