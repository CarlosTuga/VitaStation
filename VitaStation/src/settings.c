#include "settings.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "launcher.h"

#if defined(__vita__)
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif


static int storage_rename(const char *from,const char *to){
#if defined(__vita__)
    int result=sceIoRename(from,to);if(result<0)vs_log("persistence","rename failed",result);return result<0?-1:0;
#else
    int result=rename(from,to);if(result)vs_log("persistence","rename failed",errno);return result;
#endif
}
static int storage_remove(const char *path){
#if defined(__vita__)
    int result=sceIoRemove(path);return result<0?-1:0;
#else
    return remove(path);
#endif
}
static FILE *validated_read(const char *path);
/* Recoverable rotation: validate temporary output, preserve backup, then promote. */
static FILE *open_tmp_for_write(const char *path, char *tmp, size_t tmp_size) {
    if (snprintf(tmp, tmp_size, "%s.tmp", path) >= (int)tmp_size) return NULL;
    FILE *f=fopen(tmp,"wb");
    if(f && fputs("#VITASTATION 2\n",f)<0){fclose(f);return NULL;}
    return f;
}

static bool commit_tmp(FILE *f, const char *tmp, const char *path) {
    bool ok = !ferror(f);
    if(fputs("#END\n",f)<0)ok=false;
    if(fflush(f)!=0)ok=false;
    if (fclose(f) != 0) ok = false;
    if (!ok) {
        storage_remove(tmp);
        return false;
    }
    FILE *verify=fopen(tmp,"rb");
    if(!verify)return false;
    char end[5];bool complete=fseek(verify,-5,SEEK_END)==0 && fread(end,1,5,verify)==5 && !memcmp(end,"#END\n",5);
    if(fclose(verify)!=0)complete=false;
    if(!complete){vs_log("persistence","temporary validation failed",-1);return false;}
    char bak[320];snprintf(bak,sizeof(bak),"%s.bak",path);
    FILE *old=validated_read(path);bool had_valid_old=old!=NULL;
    if(old){
        fclose(old);
        FILE *previous=fopen(bak,"rb");
        if(previous){fclose(previous);if(storage_remove(bak)!=0)return false;}
        if(storage_rename(path,bak)!=0)return false;
    }else{
        /* Keep a valid backup if the primary generation was corrupt. */
        FILE *bad=fopen(path,"rb");if(bad){fclose(bad);if(storage_remove(path)!=0)return false;}
    }
    if(storage_rename(tmp,path)!=0){if(had_valid_old)storage_rename(bak,path);return false;}

    return true; /* recoverable rotation; no claim of filesystem atomicity */
}

/* Read committed data, falling back to the previous generation. */
static FILE *validated_read(const char *path){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    char head[16]={0};size_t n=fread(head,1,15,f);
    bool ok=true;
    if(n>=14 && !memcmp(head,"#VITASTATION 2",14)){
        char end[5];ok=fseek(f,-5,SEEK_END)==0 && fread(end,1,5,f)==5 && !memcmp(end,"#END\n",5);
    }
    if(!ok || fseek(f,0,SEEK_SET)!=0){fclose(f);return NULL;}return f;
}
static FILE *open_for_read(const char *path) {
    FILE *f=validated_read(path);if(f)return f;
    char bak[320];if(snprintf(bak,sizeof(bak),"%s.bak",path)>=(int)sizeof(bak))return NULL;
    return validated_read(bak); /* Uncommitted temporary files are never trusted. */
}

void settings_ensure_dir(const char *dir_path) {
    if (!dir_path) return;
#if defined(__vita__)
    sceIoMkdir(dir_path, 0777);
#else
    #if defined(_WIN32)
    mkdir(dir_path);
    #else
    mkdir(dir_path, 0777);
    #endif
#endif
}

void settings_load(UIContext *ui, const char *config_path) {
    if (!ui || !config_path) return;

    FILE *f = open_for_read(config_path);
    if (!f) {vs_log("persistence","file unavailable",-1);return;}

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char key[64], val[64];
        if (sscanf(line, "%63[^=]=%63s", key, val) == 2) {
            if (strcmp(key,"store_id")==0) { if(launcher_title_id_valid(val) || !strcmp(val,"default")) snprintf(ui->store_id,sizeof(ui->store_id),"%.9s",strcmp(val,"default")?val:"");
            } else if(!strcmp(key,"environment_theme") || !strcmp(key,"text_theme")){
                if(strlen(val)==1 && val[0]>='0' && val[0]<='4'){if(!strcmp(key,"environment_theme"))ui->environment_theme=val[0]-'0';else ui->text_theme=val[0]-'0';}
            } else if(!strcmp(key,"high_contrast")){
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->high_contrast=!strcmp(val,"1");
            } else if (strcmp(key, "sound_enabled") == 0) {
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->sound_enabled = !strcmp(val,"1");
            } else if (strcmp(key, "music_enabled") == 0) {
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->music_enabled = !strcmp(val,"1");
            } else if(strcmp(key,"auto_dlcs_enabled")==0){
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->auto_dlcs_enabled=!strcmp(val,"1");
            } else if(strcmp(key,"ps_routing_enabled")==0){
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->ps_routing_enabled=!strcmp(val,"1");
            } else if (strcmp(key, "animations_enabled") == 0) {
                if(!strcmp(val,"0") || !strcmp(val,"1"))ui->animations_enabled = !strcmp(val,"1");
            }
        }
    }

    fclose(f);
}

bool settings_save(const UIContext *ui, const char *config_path) {
    if (!ui || !config_path) return false;

#if defined(__vita__)
    settings_ensure_dir("ux0:data");
#endif
    settings_ensure_dir(VS_DATA_DIR);

    char tmp[300];
    FILE *f = open_tmp_for_write(config_path, tmp, sizeof(tmp));
    if (!f) {vs_log("persistence","file unavailable",-1);return false;}

    fprintf(f,"store_id=%s\n",ui->store_id[0]?ui->store_id:"default");
    fprintf(f, "sound_enabled=%d\n", ui->sound_enabled ? 1 : 0);
    fprintf(f, "music_enabled=%d\n", ui->music_enabled ? 1 : 0);
    fprintf(f, "animations_enabled=%d\n", ui->animations_enabled ? 1 : 0);
    fprintf(f, "ps_routing_enabled=%d\n",ui->ps_routing_enabled?1:0);
    fprintf(f, "auto_dlcs_enabled=%d\n",ui->auto_dlcs_enabled?1:0);
    fprintf(f,"environment_theme=%d\ntext_theme=%d\nhigh_contrast=%d\n",ui->environment_theme,ui->text_theme,ui->high_contrast?1:0);

    bool ok=commit_tmp(f,tmp,config_path);
    if(ok){
        char expected[512],actual[512];
        int length=snprintf(expected,sizeof(expected),"#VITASTATION 2\nstore_id=%s\nsound_enabled=%d\nmusic_enabled=%d\nanimations_enabled=%d\nps_routing_enabled=%d\nauto_dlcs_enabled=%d\nenvironment_theme=%d\ntext_theme=%d\nhigh_contrast=%d\n#END\n",ui->store_id[0]?ui->store_id:"default",ui->sound_enabled?1:0,ui->music_enabled?1:0,ui->animations_enabled?1:0,ui->ps_routing_enabled?1:0,ui->auto_dlcs_enabled?1:0,ui->environment_theme,ui->text_theme,ui->high_contrast?1:0);
        FILE *check=fopen(config_path,"rb");ok=check!=NULL && length>0 && length<(int)sizeof(expected);
        if(check){size_t read=fread(actual,1,sizeof(actual),check);if(read!=(size_t)length || memcmp(expected,actual,(size_t)length))ok=false;if(fclose(check)!=0)ok=false;}
    }
    if(!ok)vs_log("persistence","config save or readback failed",-1);
    return ok;
}

bool library_save_userdata(const Library *lib, const char *path) {
    if (!lib || !path) return false;

#if defined(__vita__)
    settings_ensure_dir("ux0:data");
#endif
    settings_ensure_dir(VS_DATA_DIR);

    char tmp[300];
    FILE *f = open_tmp_for_write(path, tmp, sizeof(tmp));
    if (!f) return false;

    for (int i = 0; i < lib->count; i++) {
        fprintf(f, "%s,%d,%d,%llu,%d,%d\n",
            lib->items[i].title_id,
            lib->items[i].is_favorite ? 1 : 0,
            lib->items[i].launch_count,
            (unsigned long long)lib->items[i].last_played, (int)lib->items[i].type, lib->items[i].classification_override?1:0);
    }

    bool ok=commit_tmp(f,tmp,path);if(!ok)vs_log("persistence","userdata save failed",-1);
    return ok;
}

void library_load_userdata(Library *lib, const char *path) {
    if (!lib || !path) return;

    FILE *f = open_for_read(path);
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char title_id[32];
        int fav = 0;
        int launches = 0, type=APP_TYPE_UNKNOWN, override=0;
        unsigned long long last_p = 0;

        int fields=sscanf(line, "%31[^,],%d,%d,%llu,%d,%d", title_id, &fav, &launches, &last_p, &type, &override);
        if (fields>=4 && launches>=0 && (fav==0 || fav==1)) {
            for (int i = 0; i < lib->count; i++) {
                if (strcmp(lib->items[i].title_id, title_id) == 0) {
                    bool manual=(fields>=6 && override==1) || (fields==5 && type==APP_TYPE_HOMEBREW);
                    if(manual && (type==APP_TYPE_VITA || type==APP_TYPE_HOMEBREW)){
                        lib->items[i].classification_override=true;lib->items[i].type=(AppType)type;
                        lib->items[i].section=type==APP_TYPE_VITA?SECTION_GAMES:SECTION_HOMEBREW;
                    }
                    lib->items[i].is_favorite = (fav != 0);
                    lib->items[i].launch_count = launches;
                    lib->items[i].last_played = (uint64_t)last_p;
                    break;
                }
            }
        }
    }

    fclose(f);
    library_update_indices(lib);
}

