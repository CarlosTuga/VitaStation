#include "game_remove.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
bool game_remove_id_allowed(const char*id){
 if(!id || strlen(id)!=9 || !strncmp(id,"NPXS",4) || !strcmp(id,"VTST00001") || !strcmp(id,"VSTUP0001") || !strcmp(id,"VSCV00001") || !strcmp(id,"PSPEMUCFW"))return false;
 for(unsigned i=0;i<9;i++)if(!isalnum((unsigned char)id[i]))return false;
 return true;
}
bool game_remove_media_target(const char*path,char*out,size_t cap,bool*directory){
 if(!path || !out || !directory || strlen(path)>=256 || strstr(path,"..") || strchr(path,'\\'))return false;
 char lower[256];size_t n=strlen(path);for(size_t i=0;i<=n;i++)lower[i]=(char)tolower((unsigned char)path[i]);
 const char*mounts[]={"ux0:","uma0:","imc0:"};
 for(unsigned m=0;m<3;m++){
  char prefix[64];snprintf(prefix,sizeof(prefix),"%spspemu/iso/",mounts[m]);size_t p=strlen(prefix);
  if(!strncmp(lower,prefix,p) && n>p+4 && !strcmp(lower+n-4,".iso")){if(n>=cap)return false;memcpy(out,path,n+1);*directory=false;return true;}
  snprintf(prefix,sizeof(prefix),"%spspemu/psp/game/",mounts[m]);p=strlen(prefix);
  if(!strncmp(lower,prefix,p) && n>p+10 && !strcmp(lower+n-10,"/eboot.pbp")){
   size_t end=n-10;if(end<=p || memchr(path+p,'/',end-p))return false;
   if(end>=cap)return false;memcpy(out,path,end);out[end]=0;*directory=true;return true;
  }
 }
 return false;
}
