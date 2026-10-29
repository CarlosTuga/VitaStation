#include "cover_pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__vita__)
#include <psp2/kernel/threadmgr.h>
#endif
#ifndef VS_COVER_PACK_ROOT
#define VS_COVER_PACK_ROOT "ux0:data/VitaStation/covers"
#endif
typedef struct {char name[96];uint32_t offset,size,crc;} Entry;
static volatile int index_lock=0;
static Entry*entries;static size_t count;static bool attempted;
static int compare(const void*a,const void*b){return strcmp(((const Entry*)a)->name,((const Entry*)b)->name);}
static void load(void){
 if(attempted)return;
 attempted=true;FILE*f=fopen(VS_COVER_PACK_ROOT "/manifest.tsv","rb");if(!f)return;
 Entry*list=calloc(32768,sizeof(Entry));if(!list){fclose(f);return;}char line[256],extra;unsigned long long size;unsigned crc;uint64_t offset=0;size_t n=0;bool ok=true;
 while(fgets(line,sizeof(line),f)){
  if(n>=32768 || sscanf(line,"%95s %llu %x %c",list[n].name,&size,&crc,&extra)!=3 || !size || size>8*1024*1024 || offset+size>0x7fffffff || strstr(list[n].name,"..") || strchr(list[n].name,'/')){ok=false;break;}
  list[n].offset=(uint32_t)offset;list[n].size=(uint32_t)size;list[n].crc=crc;offset+=size;n++;
 }
 if(ferror(f))ok=false;
 fclose(f);f=fopen(VS_COVER_PACK_ROOT "/covers.bin","rb");if(!f)ok=false;else{if(fseek(f,0,SEEK_END) || ftell(f)!=(long)offset)ok=false;fclose(f);}
 if(!ok || !n){free(list);return;}qsort(list,n,sizeof(*list),compare);entries=list;count=n;
}
bool cover_pack_find(const char*name,char*out,size_t cap){
 while(__sync_lock_test_and_set(&index_lock,1)){
#if defined(__vita__)
  sceKernelDelayThread(1000);
#endif
 }
 load();bool ok=false;
 if(entries && strlen(name)<96){Entry key;memset(&key,0,sizeof(key));strcpy(key.name,name);Entry*e=bsearch(&key,entries,count,sizeof(*entries),compare);
  if(e){int n=snprintf(out,cap,"vspack:%u:%u:%u",e->offset,e->size,e->crc);ok=n>0 && (size_t)n<cap;}
 }
 __sync_lock_release(&index_lock);return ok;
}
unsigned char*cover_pack_read(const char*path,size_t*length){
 unsigned offset,size,expected;char extra;if(sscanf(path,"vspack:%u:%u:%u%c",&offset,&size,&expected,&extra)!=3 || !size || size>8*1024*1024)return NULL;
 FILE*f=fopen(VS_COVER_PACK_ROOT "/covers.bin","rb");if(!f)return NULL;unsigned char*b=malloc(size);bool ok=b && !fseek(f,(long)offset,SEEK_SET) && fread(b,1,size,f)==size;fclose(f);
 uint32_t crc=~0u;if(ok){for(unsigned i=0;i<size;i++){crc^=b[i];for(int j=0;j<8;j++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}ok=(~crc)==expected;}
 if(!ok){free(b);return NULL;}*length=size;return b;
}

void cover_pack_reload(void){
 while(__sync_lock_test_and_set(&index_lock,1)){
#if defined(__vita__)
 sceKernelDelayThread(1000);
#endif
 }
 free(entries);entries=NULL;count=0;attempted=false;load();__sync_lock_release(&index_lock);
}
bool cover_pack_available(void){
 /* Presence probe only: the shop validates the complete index on loading it.
    Do not allocate/sort every cover or evict the live index on update checks. */
 FILE*f=fopen(VS_COVER_PACK_ROOT "/manifest.tsv","rb");if(!f)return false;
 char line[256],name[96],extra;unsigned long long size;unsigned crc;
 bool ok=fgets(line,sizeof(line),f)!=NULL;fclose(f);
 if(!ok || sscanf(line,"%95s %llu %x %c",name,&size,&crc,&extra)!=3 || !size || size>8*1024*1024 || strstr(name,"..") || strchr(name,'/'))return false;
 char path[128];snprintf(path,sizeof(path),"vspack:0:%u:%u",(unsigned)size,crc);
 size_t length=0;unsigned char*b=cover_pack_read(path,&length);bool valid=b && length==size;free(b);return valid;
}
