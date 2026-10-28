#include "bgdl_payload.h"
#include <string.h>
static bool fits(const char*s,size_t cap){if(!s)return false;for(size_t i=0;i<cap;i++){if(!s[i])return true;if((unsigned char)s[i]<32)return false;}return false;}
bool bgdl_payload_build(BgdlPayload*p,int type,const char*title,const char*url,const char*license,const char*icon){
 if(!p || (type!=0 && type!=6 && type!=12 && type!=22 && type!=23) || !fits(title,sizeof(p->title)) || !*title || !fits(url,sizeof(p->url)) || (strncmp(url,"http://",7) && strncmp(url,"https://",8)) || !fits(license,sizeof(p->license_path)) || !fits(icon,sizeof(p->icon_path)))return false;
 memset(p,0,sizeof(*p));p->type[0]=p->type[1]=type;strcpy(p->title,title);strcpy(p->url,url);strcpy(p->license_path,license);strcpy(p->icon_path,icon);return true;
}
