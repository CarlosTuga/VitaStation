#include "plugins.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
static bool same(const char*a,size_t n,const char*b){if(strlen(b)!=n)return false;for(size_t i=0;i<n;i++)if(tolower((unsigned char)a[i])!=tolower((unsigned char)b[i]))return false;return true;}
#define DISABLED_TAG "#VITASTATION_DISABLED "
static bool protected_plugin(const char*path){
    static const char*names[]={"taihen.skprx","henkaku.skprx","henkaku.suprx","storagemgr.skprx","yamt.skprx","yamt_helper.skprx","gamesd.skprx","sd2vita.skprx"};
    const char*base=strrchr(path,'/');base=base?base+1:path;
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++)if(same(base,strlen(base),names[i]))return true;
    return false;
}
size_t plugins_config_entries(const char*input,PluginEntry*entries,size_t cap){
    if(!input || !entries)return 0;
    char section[32]="";size_t count=0;
    for(const char*s=input;*s;){
        const char*end=strchr(s,'\n');if(!end)end=s+strlen(s);const char*a=s,*b=end;
        while(a<b && isspace((unsigned char)*a))a++;
        while(b>a && isspace((unsigned char)b[-1]))b--;
        if(a<b && *a=='*'){
            size_t n=(size_t)(b-a);if(n<sizeof(section)){memcpy(section,a,n);section[n]=0;}else section[0]=0;
        }else{
            bool enabled=true;if((size_t)(b-a)>=strlen(DISABLED_TAG) && !strncmp(a,DISABLED_TAG,strlen(DISABLED_TAG))){enabled=false;a+=strlen(DISABLED_TAG);}
            size_t n=(size_t)(b-a);
            if(section[0] && n>6 && n<256 && *a!='#' && memchr(a,':',n) && (same(b-6,6,".skprx") || same(b-6,6,".suprx"))){
                if(count==cap)return count;
                PluginEntry*entry=&entries[count++];memset(entry,0,sizeof(*entry));memcpy(entry->path,a,n);entry->path[n]=0;snprintf(entry->section,sizeof(entry->section),"%s",section);entry->enabled=enabled;entry->protected_entry=protected_plugin(entry->path);
            }
        }
        s=*end?end+1:end;
    }return count;
}
int plugins_config_toggle(const char*input,const PluginEntry*entry,bool enabled,char*out,size_t cap){
    if(!input || !entry || !out || input==out || !entry->path[0] || protected_plugin(entry->path) || strchr(entry->path,'\n') || strchr(entry->path,'\r'))return -1;
    char section[32]="";size_t length=strlen(input);
    for(const char*s=input;*s;){
        const char*end=strchr(s,'\n');if(!end)end=s+strlen(s);const char*a=s,*b=end;
        while(a<b && isspace((unsigned char)*a))a++;
        while(b>a && isspace((unsigned char)b[-1]))b--;
        if(a<b && *a=='*'){
            size_t n=(size_t)(b-a);if(n<sizeof(section)){memcpy(section,a,n);section[n]=0;}else section[0]=0;
        }else{
            bool current=true;if((size_t)(b-a)>=strlen(DISABLED_TAG) && !strncmp(a,DISABLED_TAG,strlen(DISABLED_TAG))){current=false;a+=strlen(DISABLED_TAG);}
            if(!strcmp(section,entry->section) && same(a,(size_t)(b-a),entry->path)){
                if(current==enabled){if(length+1>cap)return -1;memcpy(out,input,length+1);return 0;}
                const char*tail=*end?end+1:end;size_t prefix=(size_t)(s-input);char line[300];int n=snprintf(line,sizeof(line),"%s%s\n",enabled?"":DISABLED_TAG,entry->path);
                if(n<0 || (size_t)n>=sizeof(line) || prefix+(size_t)n+strlen(tail)+1>cap)return -1;
                memcpy(out,input,prefix);memcpy(out+prefix,line,(size_t)n);strcpy(out+prefix+n,tail);return 1;
            }
        }
        s=*end?end+1:end;
    }return -1;
}
int plugins_config_add(const char*input,const char*section,const char*path,char*out,size_t cap){
    if(!input || !section || !path || !out || input==out || section[0]!='*' || strchr(section,'\n') || strchr(section,'\r') || strchr(path,'\n') || strchr(path,'\r') || !strchr(path,':'))return -1;
    size_t len=strlen(input),insert=len;bool active=false,found=false;
    const char*name=strrchr(path,'/');name=name?name+1:path;
    for(const char*s=input;*s;){
        const char*end=strchr(s,'\n');if(!end)end=s+strlen(s);const char*a=s,*b=end;
        while(a<b && isspace((unsigned char)*a))a++;
        while(b>a && isspace((unsigned char)b[-1]))b--;
        if(a<b && *a=='*'){
            if(active && !found){insert=(size_t)(s-input);found=true;}
            active=same(a,(size_t)(b-a),section);
        }else if(a<b && *a!='#'){
            const char*base=a;for(const char*t=a;t<b;t++)if(*t=='/' || *t==':')base=t+1;
            if(same(base,(size_t)(b-base),name)){
                if(!active)return -1;
                if(len+1>cap)return -1;
                memcpy(out,input,len+1);return 0;
            }
        }
        s=*end?end+1:end;
    }
    bool exists=false;
    for(const char*s=input;*s;){const char*end=strchr(s,'\n');if(!end)end=s+strlen(s);const char*a=s,*b=end;while(a<b && isspace((unsigned char)*a))a++;while(b>a && isspace((unsigned char)b[-1]))b--;if(same(a,(size_t)(b-a),section))exists=true;s=*end?end+1:end;}
    char addition[512];int written=snprintf(addition,sizeof(addition),"%s%s%s%s\n",insert && input[insert-1]!='\n'?"\n":"",exists?"":section,exists?"":"\n",path);
    if(written<0 || (size_t)written>=sizeof(addition) || len+(size_t)written+1>cap)return -1;
    memcpy(out,input,insert);memcpy(out+insert,addition,(size_t)written);memcpy(out+insert+written,input+insert,len-insert+1);return 1;
}
int plugins_psp_enable(const char*input,const char*path,char*out,size_t cap){
    if(!input || !path || !out || input==out || strchr(path,'\n') || strchr(path,'\r') || strncmp(path,"ms0:/seplugins/",15))return -1;
    size_t len=strlen(input);if(len+1>cap)return -1;
    const char*requested=strrchr(path,'/');requested=requested?requested+1:path;
    for(const char*s=input;*s;){
        const char*end=strchr(s,'\n');if(!end)end=s+strlen(s);const char*a=s;while(a<end && isspace((unsigned char)*a))a++;
        if(a<end && *a!='#'){
            const char*space=a;while(space<end && !isspace((unsigned char)*space))space++;
            const char*base=a;for(const char*t=a;t<space;t++)if(*t=='/')base=t+1;
            if(same(base,(size_t)(space-base),"npdrm_free.prx") || same(base,(size_t)(space-base),"npdrm_free_mod.prx")){
                if(!same(base,(size_t)(space-base),requested))return -1;
                const char*value=space;while(value<end && isspace((unsigned char)*value))value++;
                if(value>=end || (*value!='0' && *value!='1'))return -1;
                memcpy(out,input,len+1);if(*value=='1')return 0;
                out[value-input]='1';return 1;
            }
        }
        s=*end?end+1:end;
    }
    int n=snprintf(out,cap,"%s%s%s 1\n",input,len && input[len-1]!='\n'?"\n":"",path);return n>=0 && (size_t)n<cap?1:-1;
}
#if !defined(__vita__)
bool plugins_prepare(char*message,size_t n){if(message && n)snprintf(message,n,"Plugins: instalacao apenas na PS Vita.");return false;}
size_t plugins_list(PluginEntry*entries,size_t cap){(void)entries;(void)cap;return 0;}
bool plugins_set_enabled(const PluginEntry*entry,bool enabled,char*message,size_t size){(void)entry;(void)enabled;if(message && size)snprintf(message,size,"A alteracao de plugins exige uma PS Vita.");return false;}
#endif
