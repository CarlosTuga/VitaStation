#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#define NOT_FOUND (-2)
typedef struct {long long st_size;} SceIoStat;
static int sceIoGetstat(const char*p,SceIoStat*s){struct stat st;if(stat(p,&st))return NOT_FOUND;s->st_size=st.st_size;return 0;}
static int sceIoRename(const char*a,const char*b){return rename(a,b);}
static uint64_t sceKernelGetProcessTimeWide(void){return 123;}
#include "../tools/maintenance/config_cleanup.h"
int main(void){
 const char*original="*KERNEL\n ur0:tai/other.skprx\n*main\n  ur0:tai/VitaStation/VitaStationBoot.suprx\r\nux0:tai/VitaStation/VitaStationBoot.suprx\n# ur0:tai/VitaStation/VitaStationBoot.suprx\nur0:tai/other.suprx\n";
 FILE*f=fopen("config.txt","wb");assert(f);fwrite(original,1,strlen(original),f);fclose(f);
 assert(disable_config("config.txt")==0);char actual[1024]={0};f=fopen("config.txt","rb");assert(f);fread(actual,1,sizeof(actual)-1,f);fclose(f);
 assert(!strcmp(actual,"*KERNEL\n ur0:tai/other.skprx\n*main\n# ur0:tai/VitaStation/VitaStationBoot.suprx\nur0:tai/other.suprx\n"));
 char backup[1024]={0};f=fopen("config.txt.vst-remove-123.bak","rb");assert(f);fread(backup,1,sizeof(backup)-1,f);fclose(f);assert(!strcmp(backup,original));
 assert(disable_config("config.txt")==0);assert(disable_config("absent.txt")==0);
 puts("Maintenance configuration: only owned plugin lines removed, original backed up, other plugins/comments preserved; repeat safe.");
}