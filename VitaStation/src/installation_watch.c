#include "installation_watch.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#if defined(__vita__)
#include <psp2/io/stat.h>
#else
#include <sys/stat.h>
#endif
static uint64_t mix(uint64_t h,uint64_t value){for(unsigned i=0;i<8;i++){h^=(unsigned char)(value>>(i*8));h*=UINT64_C(1099511628211);}return h;}
uint64_t installation_signature(void){
 const char*roots[]={"ux0:app","ur0:appmeta","ux0:pspemu/PSP/GAME","ux0:pspemu/ISO","uma0:pspemu/PSP/GAME","uma0:pspemu/ISO","imc0:pspemu/PSP/GAME","imc0:pspemu/ISO"};uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned i=0;i<sizeof(roots)/sizeof(roots[0]);i++){
#if defined(__vita__)
  SceIoStat st;memset(&st,0,sizeof(st));int r=sceIoGetstat(roots[i],&st);h=mix(h,r>=0);if(r>=0){h=mix(h,st.st_size);h=mix(h,st.st_mtime.year);h=mix(h,st.st_mtime.month);h=mix(h,st.st_mtime.day);h=mix(h,st.st_mtime.hour);h=mix(h,st.st_mtime.minute);h=mix(h,st.st_mtime.second);h=mix(h,st.st_mtime.microsecond);}
#else
  struct stat st;int r=stat(roots[i],&st);h=mix(h,r==0);if(!r){h=mix(h,(uint64_t)st.st_size);h=mix(h,(uint64_t)st.st_mtime);}
#endif
 }return h;
}
bool installation_cache_changed(const char*path){unsigned long long value=0;FILE*f=fopen(path,"rb");bool ok=f && fscanf(f,"VSWATCH01 %llx",&value)==1;if(f)fclose(f);return !ok || (uint64_t)value!=installation_signature();}
bool installation_cache_commit(const char*path){char temp[256];if(snprintf(temp,sizeof(temp),"%s.new",path)>=(int)sizeof(temp))return false;FILE*f=fopen(temp,"wb");if(!f)return false;bool ok=fprintf(f,"VSWATCH01 %llx\n",(unsigned long long)installation_signature())>0;if(fclose(f))ok=false;if(!ok)return false;remove(path);return !rename(temp,path);}
void installation_mark_dirty(void){FILE*f=fopen(VS_DATA_DIR "/library-dirty","wb");if(f){fputs("Rebuild after completed installation\n",f);fclose(f);}}
bool installation_dirty(void){FILE*f=fopen(VS_DATA_DIR "/library-dirty","rb");if(!f)return false;fclose(f);return true;}
void installation_clear_dirty(void){remove(VS_DATA_DIR "/library-dirty");}
