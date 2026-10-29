#include "library_cache.h"
#include "launcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#if defined(__vita__)
#include <psp2/kernel/threadmgr.h>
#endif
typedef struct {char magic[8];uint32_t item_size,count,checksum;} Header;
static uint32_t hash(const unsigned char*b,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;i++){h^=b[i];h*=16777619u;}return h;}
static bool valid(const AppItem*a){
 const unsigned char*b=(const unsigned char*)a;
 const size_t flags[]={offsetof(AppItem,classification_override),offsetof(AppItem,is_favorite),offsetof(AppItem,exists),offsetof(AppItem,media_raw)};
 for(size_t i=0;i<sizeof(flags)/sizeof(flags[0]);i++)if(b[flags[i]]>1)return false;
 if(!memchr(a->title_id,0,sizeof(a->title_id)) || !launcher_title_id_valid(a->title_id) || !strcmp(a->title_id,"VTST00001"))return false;
 return memchr(a->media_path,0,sizeof(a->media_path)) && memchr(a->game_id,0,sizeof(a->game_id)) && memchr(a->title,0,sizeof(a->title)) && memchr(a->category_str,0,sizeof(a->category_str)) && memchr(a->content_id,0,sizeof(a->content_id)) && memchr(a->icon_path,0,sizeof(a->icon_path)) && memchr(a->pic_path,0,sizeof(a->pic_path)) && (unsigned)a->type<=APP_TYPE_UNKNOWN && (unsigned)a->section<SECTION_COUNT && a->launch_count>=0;
}
bool library_cache_load(Library*lib,const char*path){
 if(!lib || !path)return false;
 FILE*f=fopen(path,"rb");if(!f)return false;Header h;Library*pending=NULL;bool ok=false;
 if(fread(&h,1,sizeof(h),f)!=sizeof(h) || memcmp(h.magic,"VSLIB001",8) || h.item_size!=sizeof(AppItem) || h.count>MAX_APPS)goto done;
 pending=calloc(1,sizeof(*pending));if(!pending)goto done;
 if(fread(pending->items,sizeof(AppItem),h.count,f)!=h.count || fgetc(f)!=EOF || ferror(f) || hash((const unsigned char*)pending->items,h.count*sizeof(AppItem))!=h.checksum)goto done;
 for(uint32_t i=0;i<h.count;i++)if(!valid(&pending->items[i]))goto done;
 pending->count=(int)h.count;library_update_indices(pending);pending->scan_complete=true;memcpy(lib,pending,sizeof(*lib));ok=true;
done:free(pending);if(fclose(f))ok=false;return ok;
}
bool library_cache_save(const Library*lib,const char*path){
 if(!lib || !path || strlen(path)>220 || lib->count<0 || lib->count>MAX_APPS)return false;
 for(int i=0;i<lib->count;i++)if(!valid(&lib->items[i]))return false;
 char temp[256],backup[256];snprintf(temp,sizeof(temp),"%s.new",path);snprintf(backup,sizeof(backup),"%s.previous",path);
 Header h;memcpy(h.magic,"VSLIB001",8);h.item_size=sizeof(AppItem);h.count=(uint32_t)lib->count;h.checksum=hash((const unsigned char*)lib->items,h.count*sizeof(AppItem));
 FILE*f=fopen(temp,"wb");if(!f)return false;bool ok=fwrite(&h,1,sizeof(h),f)==sizeof(h) && fwrite(lib->items,sizeof(AppItem),h.count,f)==h.count && fflush(f)==0;if(fclose(f))ok=false;if(!ok){remove(temp);return false;}
 f=fopen(path,"rb");bool had=f!=NULL;if(f)fclose(f);
 if(had){remove(backup);if(rename(path,backup)){remove(temp);return false;}}
 if(rename(temp,path)){if(had)rename(backup,path);return false;}
 return true;
}

/* Every second console boot. Reopening the launcher in the same boot is free.
   The boot epoch is wall-clock seconds minus kernel uptime, with RTC jitter tolerance. */
static int64_t active_boot;
static void save_counter(const char*path,unsigned count,int64_t boot){FILE*f=fopen(path,"wb");if(f){fprintf(f,"V2 %u %lld\n",count,(long long)boot);fclose(f);}}
void library_cache_reset_counter(const char*path){save_counter(path,0,active_boot);}
bool library_cache_rebuild_due_boot(bool valid_cache,const char*path,int64_t boot){
 unsigned count=0;long long previous=0;char extra;FILE*f=fopen(path,"rb");bool valid=f && fscanf(f,"V2 %u %lld %c",&count,&previous,&extra)==2 && count<=2;if(f)fclose(f);
 bool same=valid && llabs((long long)boot-previous)<=3;
 active_boot=same?previous:boot;
 if(!valid_cache || !valid)count=2;else if(!same && count<2)count++;
 save_counter(path,count,active_boot);return count>=2;
}
bool library_cache_rebuild_due(bool valid_cache,const char*path){
#if defined(__vita__)
 int64_t boot=(int64_t)time(NULL)-(int64_t)(sceKernelGetSystemTimeWide()/1000000);
#else
 int64_t boot=1000;
#endif
 return library_cache_rebuild_due_boot(valid_cache,path,boot);
}
