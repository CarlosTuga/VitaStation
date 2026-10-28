#include "autoboot.h"
#include "settings.h"
#include "plugins.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__vita__)
#include <psp2/io/fcntl.h>
#endif
static int rename_file(const char*a,const char*b){
#if defined(__vita__)
    return sceIoRename(a,b);
#else
    return rename(a,b);
#endif
}
static int remove_file(const char*path){
#if defined(__vita__)
    return sceIoRemove(path);
#else
    return remove(path);
#endif
}
bool autoboot_marker_read(const char*path){
    FILE*f=path?fopen(path,"rb"):NULL;if(!f)return false;
    char bytes[11];size_t n=fread(bytes,1,sizeof(bytes),f);bool ok=n==10 && !memcmp(bytes,"VTST00001\n",10);if(fclose(f))ok=false;return ok;
}
bool autoboot_marker_write(const char*path,bool enabled){
    if(!path || strlen(path)>240)return false;
    if(!enabled){FILE*f=fopen(path,"rb");if(!f)return true;fclose(f);if(remove_file(path)<0)return false;f=fopen(path,"rb");if(f){fclose(f);return false;}return true;}
    if(autoboot_marker_read(path))return true;
    char temp[256],old[256];snprintf(temp,sizeof(temp),"%s.new",path);snprintf(old,sizeof(old),"%s.previous",path);
    FILE*f=fopen(temp,"wb");if(!f)return false;bool ok=fwrite("VTST00001\n",1,10,f)==10 && fflush(f)==0;if(fclose(f))ok=false;
    if(!ok || !autoboot_marker_read(temp)){remove_file(temp);return false;}
    f=fopen(path,"rb");bool had=f!=NULL;if(f)fclose(f);
    if(had){f=fopen(old,"rb");if(f){fclose(f);return false;}if(rename_file(path,old)<0)return false;}
    if(rename_file(temp,path)<0){if(had)rename_file(old,path);return false;}
    if(!autoboot_marker_read(path)){remove_file(path);if(had)rename_file(old,path);return false;}
    if(had)remove_file(old);
    return true;
}
bool autoboot_is_enabled(void){return autoboot_marker_read(VS_DATA_DIR "/autoboot.enabled");}
bool autoboot_bridge_available(void){
#if defined(__vita__)
    {
        const char*root="ur0:tai";FILE*f=fopen("ux0:tai/config.txt","rb");if(f){fclose(f);root="ux0:tai";}
        char path[256];snprintf(path,sizeof(path),"%s/VitaStation/VitaStationBoot.suprx",root);
        f=fopen(path,"rb");if(!f)return false;fclose(f);
        snprintf(path,sizeof(path),"%s/config.txt",root);f=fopen(path,"rb");if(!f)return false;
        char*input=malloc(128*1024),*output=malloc(128*1024);if(!input || !output){free(input);free(output);fclose(f);return false;}
        size_t n=fread(input,1,128*1024-1,f);bool ok=!ferror(f) && feof(f);fclose(f);input[n]=0;
        snprintf(path,sizeof(path),"%s/VitaStation/VitaStationBoot.suprx",root);
        ok=ok && plugins_config_add(input,"*main",path,output,128*1024)==0;free(input);free(output);if(!ok)return false;
    }
    return true;
#else
    return false;
#endif
}
bool autoboot_set(bool enabled){
#if defined(__vita__)
    if(enabled && !autoboot_bridge_available())return false;
    settings_ensure_dir("ux0:data");settings_ensure_dir(VS_DATA_DIR);return autoboot_marker_write(VS_DATA_DIR "/autoboot.enabled",enabled);
#else
    (void)enabled;return false;
#endif
}
