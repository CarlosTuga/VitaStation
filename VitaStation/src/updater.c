#include "updater.h"
#include "version.h"
#include "../third_party/cJSON/cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned version(const char*s){unsigned a=0,b=0,c=0;if(*s=='v')s++;if(sscanf(s,"%u.%u.%u",&a,&b,&c)!=3 || a>99 || b>99 || c>99)return 0;return a*10000+b*100+c;}
static const char*text(const cJSON*j,const char*k){const cJSON*v=cJSON_GetObjectItemCaseSensitive(j,k);return cJSON_IsString(v)?v->valuestring:"";}
bool update_select(const char*json,size_t length,UpdateStatus*out){
 cJSON*root=cJSON_ParseWithLength(json,length);if(!root || !cJSON_IsArray(root)){cJSON_Delete(root);return false;}
 memset(out,0,sizeof(*out));unsigned best=version(VS_VERSION);out->phase=4;snprintf(out->message,sizeof(out->message),"VitaStation " VS_VERSION_LABEL ": atualizado.");
 const cJSON*r; cJSON_ArrayForEach(r,root){
  if(!cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(r,"draft")))continue;
  unsigned v=version(text(r,"tag_name"));if(v<=best)continue;
  const cJSON*a;cJSON_ArrayForEach(a,cJSON_GetObjectItemCaseSensitive(r,"assets")){
   const char*n=text(a,"name"),*url=text(a,"browser_download_url"),*d=text(a,"digest");size_t len=strlen(n);
   if(strncmp(n,"VitaStation-",12) || len<5 || strcmp(n+len-4,".vpk"))continue;
   if(strncmp(url,"https://github.com/CarlosTuga/VitaStation/releases/download/",strlen("https://github.com/CarlosTuga/VitaStation/releases/download/")))continue;
   if(strncmp(d,"sha256:",7) || strlen(d)!=71 || strspn(d+7,"0123456789abcdefABCDEF")!=64)continue;
   const cJSON*s=cJSON_GetObjectItemCaseSensitive(a,"size");if(!cJSON_IsNumber(s) || s->valuedouble<1 || s->valuedouble>64*1024*1024)continue;
   if(strlen(url)>=sizeof(out->url) || strlen(text(r,"tag_name"))>=sizeof(out->version))continue;
   best=v;out->phase=2;out->size=(unsigned long)s->valuedouble;snprintf(out->version,sizeof(out->version),"%s",text(r,"tag_name"));snprintf(out->url,sizeof(out->url),"%s",url);snprintf(out->digest,sizeof(out->digest),"%s",d+7);snprintf(out->message,sizeof(out->message),"Nova versao %s. X: descarregar VPK.",out->version);break;
  }
 }
 cJSON_Delete(root);return true;
}
