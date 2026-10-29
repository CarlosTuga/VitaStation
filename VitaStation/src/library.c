#include "library.h"
#include "psp_media.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "log.h"

/* O proprio launcher nao deve aparecer na biblioteca. */
#define SELF_TITLE_ID "VTST00001"

#if defined(__vita__)
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

uint64_t library_date(int year,int month,int day,int hour,int minute,int second){
 if(year<1970 || year>9999 || month<1 || month>12 || day<1 || day>31 || hour<0 || hour>23 || minute<0 || minute>59 || second<0 || second>60)return 0;
 int y=year-(month<=2),era=y/400,yoe=y-era*400,mp=month+(month>2?-3:9);int64_t days=(int64_t)era*146097+yoe*365+yoe/4-yoe/100+(153*mp+2)/5+day-1-719468;
 return days<0?0:(uint64_t)days*86400+(uint64_t)hour*3600+(unsigned)minute*60+(unsigned)second;
}
uint64_t library_file_date(const char*path){
#if defined(__vita__)
 SceIoStat st;if(sceIoGetstat(path,&st)<0)return 0;SceDateTime*t=st.st_ctime.year>=1970?&st.st_ctime:&st.st_mtime;return library_date(t->year,t->month,t->day,t->hour,t->minute,t->second);
#else
 struct stat st;if(stat(path,&st))return 0;return st.st_ctime>0?(uint64_t)st.st_ctime:(uint64_t)st.st_mtime;
#endif
}
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t key_table_offset;
    uint32_t data_table_offset;
    uint32_t num_entries;
} SFOHeader;

typedef struct {
    uint16_t key_offset;
    uint8_t  data_fmt;
    uint8_t  reserved;
    uint32_t data_len;
    uint32_t data_max_len;
    uint32_t data_offset;
} SFOEntry;

void library_init(Library *lib) {
    if (!lib) return;
    memset(lib, 0, sizeof(Library));
}

/* Read bounded bytes, never cast untrusted input to native structures. */
static uint32_t le32(const unsigned char *b) { return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24); }
static int parse_sfo(const char *path, char *title, size_t ts, char *cat, size_t cs, char *id, size_t ids, char *content, size_t content_size) {
    if(id && ids)id[0]=0;
    if(content && content_size)content[0]=0;
    if(title && ts) title[0]=0;
    if(cat && cs) cat[0]=0;
    FILE *f=path?fopen(path,"rb"):NULL;
    if(!f) return -1;
    int ok=-1; unsigned char *b=NULL;
    if(fseek(f,0,SEEK_END)!=0) goto done;
    long n=ftell(f);
    if(n<20 || n>1024*1024 || fseek(f,0,SEEK_SET)!=0) goto done;
    b=malloc((size_t)n); if(!b || fread(b,1,(size_t)n,f)!=(size_t)n) goto done;
    uint32_t k=le32(b+8),d=le32(b+12),count=le32(b+16);
    if(le32(b)!=0x46535000 || le32(b+4)!=0x101 || !count || count>256 ||
       20+count*16>k || k>d || d>(uint32_t)n) goto done;
    for(uint32_t i=0;i<count;i++) {
        unsigned char *e=b+20+i*16;
        uint32_t ko=e[0]|((uint32_t)e[1]<<8),len=le32(e+4),max=le32(e+8),off=le32(e+12);
        if(ko>=d-k || !memchr(b+k+ko,0,d-k-ko) || len>max || off>(uint32_t)n-d || max>(uint32_t)n-d-off) goto done;
        unsigned fmt=e[2]|((unsigned)e[3]<<8);
        if(fmt==0x204) {
            if(!len || !memchr(b+d+off,0,len)) goto done;
            char *out=NULL; size_t cap=0;
            if(!strcmp((char*)b+k+ko,"TITLE")){out=title;cap=ts;}
            if(!strcmp((char*)b+k+ko,"CATEGORY")){out=cat;cap=cs;}
            if(!strcmp((char*)b+k+ko,"TITLE_ID")){out=id;cap=ids;}
            if(!strcmp((char*)b+k+ko,"CONTENT_ID")){out=content;cap=content_size;}
            if(out && cap) snprintf(out,cap,"%s",(char*)b+d+off);
        } else if(fmt!=0x404 && fmt!=4) goto done;
    }
    ok=0;
done:
    free(b); if(fclose(f)!=0) ok=-1;
    if(ok){if(id&&ids)id[0]=0;if(title&&ts)title[0]=0;if(cat&&cs)cat[0]=0;}
    return ok;
}

int library_parse_param_sfo(const char *path,char *title,size_t ts,char *cat,size_t cs){return parse_sfo(path,title,ts,cat,cs,NULL,0,NULL,0);}

static bool readable_image(const char*path){
    unsigned char bytes[8];FILE*f=fopen(path,"rb");if(!f)return false;
    size_t n=fread(bytes,1,sizeof(bytes),f);fclose(f);
    return (n==8 && !memcmp(bytes,"\x89PNG\r\n\x1a\n",8)) || (n>=3 && bytes[0]==0xff && bytes[1]==0xd8 && bytes[2]==0xff);
}
static void find_livearea_background(const char*base,char*out,size_t capacity){
 char directory[256];if(snprintf(directory,sizeof(directory),"%s/livearea/contents",base)>=(int)sizeof(directory))return;
 unsigned best=0;
#if defined(__vita__)
 SceUID d=sceIoDopen(directory);if(d<0)return;while(1){SceIoDirent entry;memset(&entry,0,sizeof(entry));int n=sceIoDread(d,&entry);if(n<=0)break;const char*name=entry.d_name;
#else
 DIR*d=opendir(directory);if(!d)return;struct dirent*entry;while((entry=readdir(d))){const char*name=entry->d_name;
#endif
 size_t length=strlen(name);if(length<5 || strstr(name,"..") || strcmp(name+length-4,".png"))continue;char path[256];if(snprintf(path,sizeof(path),"%s/%s",directory,name)>=(int)sizeof(path))continue;
 unsigned char h[24];FILE*f=fopen(path,"rb");if(!f)continue;size_t nread=fread(h,1,sizeof(h),f);fclose(f);if(nread!=24 || memcmp(h,"\x89PNG\r\n\x1a\n",8))continue;
 unsigned w=(unsigned)h[16]<<24|(unsigned)h[17]<<16|(unsigned)h[18]<<8|h[19],height=(unsigned)h[20]<<24|(unsigned)h[21]<<16|(unsigned)h[22]<<8|h[23];
 if(w>=480 && w<=1920 && height>=200 && height<=1088 && w*height>best){best=w*height;snprintf(out,capacity,"%s",path);}
 }
#if defined(__vita__)
 sceIoDclose(d);
#else
 closedir(d);
#endif
}
static bool metadata(AppItem*item,const char*root,const char*id){
    if(!strcmp(id,"VSTUP0001"))return false;
    char bases[3][MAX_PATH_LEN];unsigned count=0;
#if defined(__vita__)
    /* System appmeta exposes artwork without mounting another title's encrypted PFS. */
    snprintf(bases[count++],MAX_PATH_LEN,"ur0:appmeta/%s",id);
    snprintf(bases[count++],MAX_PATH_LEN,"ux0:appmeta/%s",id);
#endif
    int written=snprintf(bases[count],MAX_PATH_LEN,"%s/%s/sce_sys",root,id);if(written<0 || written>=MAX_PATH_LEN)return false;count++;
    char path[MAX_PATH_LEN],metadata_id[32];bool valid=false;
    for(unsigned i=0;i<count;i++){
        written=snprintf(path,sizeof(path),"%s/param.sfo",bases[i]);if(written<0 || written>=(int)sizeof(path))continue;
        if(!parse_sfo(path,item->title,sizeof(item->title),item->category_str,sizeof(item->category_str),metadata_id,sizeof(metadata_id),item->content_id,sizeof(item->content_id)) && (!metadata_id[0] || !strcmp(metadata_id,id))){valid=true;break;}
    }
    if(!valid)return false;
    item->icon_path[0]=item->pic_path[0]=0;
    const char*icons[]={"icon0.png","icon0.jpg"};const char*pictures[]={"pic0.png","pic1.png","pic0.jpg","pic1.jpg","livearea/contents/bg.png"};
    for(unsigned i=0;i<count;i++){
        for(unsigned k=0;k<2 && !item->icon_path[0];k++){
            written=snprintf(path,sizeof(path),"%s/%s",bases[i],icons[k]);if(written>0 && written<(int)sizeof(path) && readable_image(path))snprintf(item->icon_path,sizeof(item->icon_path),"%s",path);
        }
        for(unsigned k=0;k<5 && !item->pic_path[0];k++){
            written=snprintf(path,sizeof(path),"%s/%s",bases[i],pictures[k]);if(written>0 && written<(int)sizeof(path) && readable_image(path))snprintf(item->pic_path,sizeof(item->pic_path),"%s",path);
        }
    }
    if(!item->pic_path[0])for(unsigned i=0;i<count && !item->pic_path[0];i++)find_livearea_background(bases[i],item->pic_path,sizeof(item->pic_path));
    if(!item->icon_path[0]){
        const char*cover_roots[]={"ux0:data/VitaStation/covers","ux0:usagi-pkgj/cover","ur0:usagi-pkgj/cover","ux0:pkgj/cover"};
        const char*suffix[]={".png",".cover.jpg",".jpg"};
        for(unsigned i=0;i<4 && !item->icon_path[0];i++)for(unsigned k=0;k<3 && !item->icon_path[0];k++){
            written=snprintf(path,sizeof(path),"%s/%s%s",cover_roots[i],id,suffix[k]);
            if(written>0 && written<(int)sizeof(path) && readable_image(path))snprintf(item->icon_path,sizeof(item->icon_path),"%s",path);
        }
    }
    char installed_path[256];if(snprintf(installed_path,sizeof(installed_path),"%s/%s",root,id)<(int)sizeof(installed_path))item->installed_at=library_file_date(installed_path);
    if(!item->title[0])snprintf(item->title,sizeof(item->title),"%s",id);
    return true;
}

void library_update_indices(Library *lib) {
    if (!lib) return;
    lib->games_count = 0;
    lib->media_count = 0;

    for (int i = 0; i < lib->count; i++) {
        if (lib->items[i].section == SECTION_GAMES) {
            lib->games_indices[lib->games_count++] = i;
        } else {
            lib->media_indices[lib->media_count++] = i;
        }
    }
}

bool library_add_app(Library *lib, const AppItem *app) {
    if (!lib || !app || lib->count >= MAX_APPS) return false;

    for (int i = 0; i < lib->count; i++) {
        if (strcmp(lib->items[i].title_id, app->title_id) == 0) {
            lib->items[i] = *app;
            library_update_indices(lib);
            return true;
        }
    }

    lib->scan_complete=true;
    lib->items[lib->count] = *app;
    lib->count++;
    library_update_indices(lib);
    return true;
}

static int scan_internal(Library *lib, const char *root_path) {
    if (!lib) return -1;
    const char *scan_dir = root_path ? root_path : "ux0:app/";
    char normalized[MAX_PATH_LEN];size_t root_len=strlen(scan_dir);
    if(!root_len || root_len>=sizeof(normalized))return -1;
    memcpy(normalized,scan_dir,root_len+1);
    if(root_len>1 && normalized[root_len-1]=='/')normalized[root_len-1]=0;
    scan_dir=normalized;

#if defined(__vita__)
    SceUID probe=sceIoDopen(scan_dir);if(probe<0){vs_log("library","root unavailable",probe);return -1;}sceIoDclose(probe);
#else
    DIR *probe=opendir(scan_dir);if(!probe){vs_log("library","root unavailable",-1);return -1;}closedir(probe);
#endif
    // Preserve favorites/stats before re-scanning
    // (no heap: um array de MAX_APPS AppItem na pilha ocupa ~184 KB)
    int old_count = lib->count;
    AppItem *old_items = old_count > 0 ? (AppItem *)malloc(sizeof(AppItem) * (size_t)old_count) : NULL;
    if (old_items) {
        memcpy(old_items, lib->items, sizeof(AppItem) * (size_t)old_count);
    } else if(old_count) {
        return -1; /* Preserve metadata on allocation failure. */
    }

    lib->count = 0;
    lib->games_count = 0;
    lib->media_count = 0;

#if defined(__vita__)
    SceUID dfd = sceIoDopen(scan_dir);
    if(dfd<0){free(old_items);return -1;}
    if (dfd >= 0) {
        SceIoDirent dir;
        /* SceIoDirent tem de estar a zero antes de cada sceIoDread */
        int read_result;
        while (memset(&dir, 0, sizeof(dir)), (read_result=sceIoDread(dfd, &dir)) > 0) {
            if (dir.d_name[0] == '.' || strcmp(dir.d_name, SELF_TITLE_ID) == 0 || strlen(dir.d_name) != 9) continue;
            bool valid=true;for(int n=0;n<9;n++)if(!isalnum((unsigned char)dir.d_name[n]))valid=false;
            if(!valid)continue;
            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                AppItem item;
                memset(&item, 0, sizeof(AppItem));
                snprintf(item.title_id, sizeof(item.title_id), "%.31s", dir.d_name);

                if(!metadata(&item,scan_dir,dir.d_name)){vs_log("sfo","invalid application metadata",-1);continue;}

                /* Unidentified and unofficial apps belong to Homebrews by default.
                   Official-game membership is checked against the local Vita catalogs. */
                item.section = SECTION_HOMEBREW;
                item.type = APP_TYPE_HOMEBREW;
                item.exists = true;
                psp_identify_bubble(&item,scan_dir);

                // Restore previous metadata (favorites, launch count)
                for (int j = 0; j < old_count; j++) {
                    if (strcmp(old_items[j].title_id, item.title_id) == 0) {
                        item.classification_override=old_items[j].classification_override;
                        if(item.classification_override){item.type=old_items[j].type;item.section=old_items[j].section;}
                        item.is_favorite = old_items[j].is_favorite;
                        item.launch_count = old_items[j].launch_count;
                        item.last_played = old_items[j].last_played;
                        break;
                    }
                }

                library_add_app(lib, &item);
            }
        }
        int close_result=sceIoDclose(dfd);
        if(read_result<0 || close_result<0){free(old_items);return -1;}
    }
#else
    DIR *d = opendir(scan_dir);
    if(!d){free(old_items);return -1;}
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.' || strcmp(dir->d_name, SELF_TITLE_ID) == 0 || strlen(dir->d_name) != 9) continue;

            bool valid=true;for(int n=0;n<9;n++)if(!isalnum((unsigned char)dir->d_name[n]))valid=false;
            if(!valid)continue;
            char full_path[MAX_PATH_LEN];
            snprintf(full_path, sizeof(full_path) - 1, "%.200s/%.31s", scan_dir, dir->d_name);
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                AppItem item;
                memset(&item, 0, sizeof(AppItem));
                snprintf(item.title_id, sizeof(item.title_id), "%.31s", dir->d_name);

                if(!metadata(&item,scan_dir,dir->d_name)){vs_log("sfo","invalid application metadata",-1);continue;}

                /* Unidentified and unofficial apps belong to Homebrews by default.
                   Official-game membership is checked against the local Vita catalogs. */
                item.section = SECTION_HOMEBREW;
                item.type = APP_TYPE_HOMEBREW;
                item.exists = true;
                psp_identify_bubble(&item,scan_dir);

                for (int j = 0; j < old_count; j++) {
                    if (strcmp(old_items[j].title_id, item.title_id) == 0) {
                        item.classification_override=old_items[j].classification_override;
                        if(item.classification_override){item.type=old_items[j].type;item.section=old_items[j].section;}
                        item.is_favorite = old_items[j].is_favorite;
                        item.launch_count = old_items[j].launch_count;
                        item.last_played = old_items[j].last_played;
                        break;
                    }
                }

                library_add_app(lib, &item);
            }
        }
        closedir(d);
    }
#endif

    free(old_items);
    return lib->count;
}

int library_scan(Library *lib,const char *root){
    if(!lib)return -1;
    Library *pending=malloc(sizeof(*pending));if(!pending)return -1;
    memcpy(pending,lib,sizeof(*pending));int result=scan_internal(pending,root);
    if(result>=0){
        pending->scan_complete=true;
        static const char*roots[]={"ux0:usagi-pkgj","ur0:usagi-pkgj","ux0:pkgj","ur0:pkgj"};
        char games[256]="",demos[256]="";
        for(unsigned i=0;i<4;i++){
            char path[256];FILE*f;
            if(!games[0]){snprintf(path,sizeof(path),"%s/titles_psvgames.tsv",roots[i]);f=fopen(path,"rb");if(f){fclose(f);snprintf(games,sizeof(games),"%s",path);}}
            if(!demos[0]){snprintf(path,sizeof(path),"%s/titles_psvdemos.tsv",roots[i]);f=fopen(path,"rb");if(f){fclose(f);snprintf(demos,sizeof(demos),"%s",path);}}
        }
        library_classify_catalog(pending,games,demos);
        psp_livearea_scan(pending);
#if defined(__vita__)
        const char*media_roots[]={"ux0:pspemu/ISO","ux0:pspemu/PSP/GAME","uma0:pspemu/ISO","uma0:pspemu/PSP/GAME","imc0:pspemu/ISO","imc0:pspemu/PSP/GAME"};
        for(unsigned i=0;i<sizeof(media_roots)/sizeof(media_roots[0]);i++)psp_media_scan(pending,media_roots[i]);
#endif
        memcpy(lib,pending,sizeof(*lib));result=lib->count;
    }
    free(pending);return result;
}

AppItem* library_get_app(Library *lib, int global_index) {
    if (!lib || global_index < 0 || global_index >= lib->count) return NULL;
    return &lib->items[global_index];
}

static int section_indices(Library *lib, AppSection section, int *idx) {
    int n=0;
    for(int i=0;i<lib->count;i++) {
        AppItem *a=&lib->items[i];
        if(!a->exists) continue;
        if((section==SECTION_GAMES && a->section==SECTION_GAMES) ||
           (section==SECTION_HOMEBREW && a->section==SECTION_HOMEBREW) ||
           (section==SECTION_PSP_PS1 && a->section==SECTION_PSP_PS1) ||
           (section==SECTION_RECENT && a->last_played) ||
           (section==SECTION_FAVORITES && a->is_favorite)) idx[n++]=i;
    }
    for(int i=1;i<n;i++){
        int v=idx[i],j=i;const AppItem*selected=&lib->items[v];
        while(j>0){const AppItem*previous=&lib->items[idx[j-1]];uint64_t a=section==SECTION_RECENT?previous->last_played:previous->installed_at,b=section==SECTION_RECENT?selected->last_played:selected->installed_at;
            bool before=a<b || (a==b && strcmp(previous->title,selected->title)>0) || (a==b && !strcmp(previous->title,selected->title) && strcmp(previous->title_id,selected->title_id)>0);
            if(!before)break;
            idx[j]=idx[j-1];j--;
        }idx[j]=v;
    }
    return n;
}
int library_section_count(Library *lib, AppSection s){int idx[MAX_APPS];return lib?section_indices(lib,s,idx):0;}
AppItem* library_get_section_app(Library *lib, AppSection s, int i) {
    int idx[MAX_APPS]; if(!lib || i<0) return NULL;
    int n=section_indices(lib,s,idx);return i<n?&lib->items[idx[i]]:NULL;
}

void library_toggle_favorite(Library *lib, const char *title_id) {
    if (!lib || !title_id) return;
    for (int i = 0; i < lib->count; i++) {
        if (strcmp(lib->items[i].title_id, title_id) == 0) {
            lib->items[i].is_favorite = !lib->items[i].is_favorite;
            break;
        }
    }
}

void library_record_launch(Library *lib, const char *title_id) {
    if (!lib || !title_id) return;
    for (int i = 0; i < lib->count; i++) {
        if (strcmp(lib->items[i].title_id, title_id) == 0) {
            if(lib->items[i].launch_count<INT_MAX)lib->items[i].launch_count++;
            lib->items[i].last_played = (uint64_t)time(NULL);
            break;
        }
    }
}

void library_classify_catalog(Library*lib,const char*games,const char*demos){
    if(!lib)return;
    const char*paths[]={games,demos};
    bool available=false;
    for(unsigned p=0;p<2;p++){
        FILE*f=paths[p] && paths[p][0]?fopen(paths[p],"rb"):NULL;if(!f)continue;
        if(!available){for(int i=0;i<lib->count;i++)if(!lib->items[i].classification_override && lib->items[i].type!=APP_TYPE_PSP && lib->items[i].type!=APP_TYPE_PS1){lib->items[i].type=APP_TYPE_HOMEBREW;lib->items[i].section=SECTION_HOMEBREW;}available=true;}
        char line[8192];size_t bytes=0;
        while(fgets(line,sizeof(line),f)){
            size_t length=strlen(line);bytes+=length;if(bytes>16*1024*1024)break;
            if(length==sizeof(line)-1 && line[length-1]!='\n'){int ch;while((ch=fgetc(f))!=EOF && ch!='\n'){}continue;}
            char*columns[7];unsigned count=1;columns[0]=line;
            for(char*s=line;*s;s++){if(*s=='\t'){*s=0;if(count<7)columns[count++]=s+1;}else if(*s=='\r'||*s=='\n'){*s=0;break;}}
            if(count<6 || strlen(columns[0])!=9)continue;
            for(int i=0;i<lib->count;i++){
                AppItem*app=&lib->items[i];if(app->classification_override || strcmp(app->title_id,columns[0]))continue;
                if(app->content_id[0] && strcmp(app->content_id,columns[5]))continue;
                if(strcmp(app->category_str,"gd") && app->category_str[0])continue;
                app->type=APP_TYPE_VITA;app->section=SECTION_GAMES;
            }
        }
        fclose(f);
    }
    library_update_indices(lib);
}
