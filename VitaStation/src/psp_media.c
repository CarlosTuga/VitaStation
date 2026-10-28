#include "psp_media.h"
#include "cover_pack.h"
#include "settings.h"
#include "launcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(__vita__)
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/sysmodule.h>
#include <psp2/sqlite.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
static uint32_t le(const unsigned char*b){return b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
static bool sfo(const unsigned char*b,size_t n,const char*key,char*out,size_t cap){
 if(n<20 || le(b)!=0x46535000 || le(b+4)!=0x101)return false;
 uint32_t k=le(b+8),d=le(b+12),count=le(b+16);if(count>256 || k<20+count*16 || d<k || d>n)return false;
 for(uint32_t i=0;i<count;i++){const unsigned char*e=b+20+i*16;unsigned ko=e[0]|(e[1]<<8);uint32_t len=le(e+4),offset=le(e+12);if(ko>=d-k || !memchr(b+k+ko,0,d-k-ko) || offset>n-d || len>n-d-offset)return false;if(!strcmp((const char*)b+k+ko,key) && e[2]==4 && e[3]==2 && len && memchr(b+d+offset,0,len)){snprintf(out,cap,"%s",b+d+offset);return true;}}
 return false;
}
static bool slice(FILE*f,uint64_t off,unsigned size,unsigned char**out){
 if(!size || size>1024*1024 || off>0x7fffffff || fseek(f,(long)off,SEEK_SET))return false;
 *out=malloc(size);if(!*out)return false;if(fread(*out,1,size,f)!=size){free(*out);*out=NULL;return false;}return true;
}
static bool iso_find(FILE*f,uint32_t sector,uint32_t bytes,const char*name,uint32_t*out_sector,uint32_t*out_size){
 unsigned char*b=NULL;if(!slice(f,(uint64_t)sector*2048,bytes,&b))return false;bool found=false;
 for(uint32_t at=0;at<bytes;){unsigned len=b[at];if(!len){at=(at/2048+1)*2048;continue;}if(len<34 || len>bytes-at)break;unsigned namesize=b[at+32];if(namesize>len-33)break;size_t wanted=strlen(name);if((namesize==wanted || (namesize==wanted+2 && b[at+33+wanted]==';' && b[at+34+wanted]=='1')) && !memcmp(b+at+33,name,wanted)){*out_sector=le(b+at+2);*out_size=le(b+at+10);found=true;break;}at+=len;}
 free(b);return found;
}
static void artwork(AppItem*item){
 if(!item->game_id[0])return;
 const char*suffix[]={".png",".cover.jpg",".jpg"};
 for(int i=0;i<3;i++){char name[80];snprintf(name,sizeof(name),"%s%s",item->game_id,suffix[i]);if(cover_pack_find(name,item->icon_path,sizeof(item->icon_path)))return;char path[256];snprintf(path,sizeof(path),VS_DATA_DIR "/covers/%s",name);FILE*f=fopen(path,"rb");if(f){fclose(f);snprintf(item->icon_path,sizeof(item->icon_path),"%s",path);return;}}
}
bool psp_read_metadata(const char*path,AppItem*item){
 FILE*f=fopen(path,"rb");if(!f)return false;unsigned char header[40],*metadata=NULL;bool ok=false;uint32_t icon_at=0,icon_size=0;
 if(fread(header,1,40,f)!=40)goto done;
 uint32_t start=le(header+8),end=le(header+12);
 if(le(header)==0x50425000){
  if(start<40 || end<=start || !slice(f,start,end-start,&metadata))goto done;
  sfo(metadata,end-start,"TITLE",item->title,sizeof(item->title));sfo(metadata,end-start,"DISC_ID",item->game_id,sizeof(item->game_id));sfo(metadata,end-start,"CATEGORY",item->category_str,sizeof(item->category_str));free(metadata);metadata=NULL;
  if(!strcmp(item->category_str,"ME"))item->type=APP_TYPE_PS1;else item->type=APP_TYPE_PSP;
  uint32_t next=le(header+16);if(next>end && next-end<=1024*1024){icon_at=end;icon_size=next-end;}ok=true;
 }else{
  unsigned char pvd[2048];if(fseek(f,16*2048,SEEK_SET) || fread(pvd,1,2048,f)!=2048 || pvd[0]!=1 || memcmp(pvd+1,"CD001",5))goto done;
  uint32_t sector=le(pvd+158),size=le(pvd+166),dir,dir_size,sfo_sector,sfo_size;
  if(!iso_find(f,sector,size,"PSP_GAME",&dir,&dir_size) || !iso_find(f,dir,dir_size,"PARAM.SFO",&sfo_sector,&sfo_size) || !slice(f,(uint64_t)sfo_sector*2048,sfo_size,&metadata))goto done;
  sfo(metadata,sfo_size,"TITLE",item->title,sizeof(item->title));sfo(metadata,sfo_size,"DISC_ID",item->game_id,sizeof(item->game_id));snprintf(item->category_str,sizeof(item->category_str),"UG");item->type=APP_TYPE_PSP;free(metadata);metadata=NULL;
  if(iso_find(f,dir,dir_size,"ICON0.PNG",&sector,&size) && sector<0x100000){icon_at=sector*2048;icon_size=size;}ok=true;
 }
 if(!item->title[0])ok=false;
 if(ok && icon_size && slice(f,icon_at,icon_size,&metadata)){
  if(icon_size>=8 && !memcmp(metadata,"\x89PNG\r\n\x1a\n",8)){
   settings_ensure_dir(VS_DATA_DIR "/media-icons");char pathbuf[256];snprintf(pathbuf,sizeof(pathbuf),VS_DATA_DIR "/media-icons/%s.png",item->title_id);FILE*out=fopen(pathbuf,"wb");if(out){bool written=fwrite(metadata,1,icon_size,out)==icon_size;if(fclose(out))written=false;if(written)snprintf(item->icon_path,sizeof(item->icon_path),"%s",pathbuf);}
  }
 }
done:free(metadata);fclose(f);if(ok && !item->icon_path[0])artwork(item);return ok;
}
static bool same_path(const char*a,const char*b){char x[256],y[256];unsigned i=0,j=0;while(*a && i<255){if(*a=='/' && i && x[i-1]==':'){a++;continue;}x[i++]=(char)tolower((unsigned char)*a++);}x[i]=0;while(*b && j<255){if(*b=='/' && j && y[j-1]==':'){b++;continue;}y[j++]=(char)tolower((unsigned char)*b++);}y[j]=0;return !strcmp(x,y);}
static void add_media(Library*lib,const char*path){
 for(int i=0;i<lib->count;i++)if(lib->items[i].media_path[0] && same_path(lib->items[i].media_path,path))return;
 AppItem item;memset(&item,0,sizeof(item));uint32_t hash=2166136261u;for(const unsigned char*c=(const unsigned char*)path;*c;c++){hash^=(unsigned char)tolower(*c);hash*=16777619u;}snprintf(item.title_id,sizeof(item.title_id),"R%08X",hash);snprintf(item.media_path,sizeof(item.media_path),"%s",path);item.media_raw=true;item.exists=true;item.installed_at=library_file_date(path);item.section=SECTION_PSP_PS1;
 if(psp_read_metadata(path,&item)){
  const char*end=strrchr(path,'/');if(end && !strcmp(end+1,"EBOOT.PBP")){
   const char*begin=end;while(begin>path && begin[-1]!='/')begin--;size_t length=(size_t)(end-begin);
   if(length==9){char candidate[10];memcpy(candidate,begin,9);candidate[9]=0;if(psp_native_installed(candidate)){snprintf(item.title_id,sizeof(item.title_id),"%s",candidate);item.media_raw=false;}}
  }
  if(item.game_id[0])for(int i=0;i<lib->count;i++)if(!lib->items[i].media_raw && lib->items[i].type==item.type && !strcmp(lib->items[i].game_id,item.game_id))return;
  library_add_app(lib,&item);
 }
}
static void walk(Library*lib,const char*root,int depth){
 if(depth>3 || lib->count>=MAX_APPS)return;
#if defined(__vita__)
 SceUID d=sceIoDopen(root);if(d<0)return;SceIoDirent e;while(memset(&e,0,sizeof(e)),sceIoDread(d,&e)>0){const char*name=e.d_name;bool directory=SCE_S_ISDIR(e.d_stat.st_mode);
#else
 DIR*d=opendir(root);if(!d)return;struct dirent*entry;while((entry=readdir(d))){const char*name=entry->d_name;
#endif
 if(name[0]=='.' || !strcmp(name,"UPDATE"))continue;
 char path[256];int written=snprintf(path,sizeof(path),"%s/%s",root,name);if(written<0 || written>=(int)sizeof(path))continue;
#if !defined(__vita__)
 struct stat st;if(stat(path,&st))continue;bool directory=S_ISDIR(st.st_mode);
#endif
 if(directory)walk(lib,path,depth+1);else{const char*extension=strrchr(name,'.');char ext[8]={0};if(extension && strlen(extension)<sizeof(ext)){for(unsigned i=0;extension[i];i++)ext[i]=(char)tolower((unsigned char)extension[i]);}if(!strcmp(ext,".iso") || !strcmp(ext,".pbp"))add_media(lib,path);}
 }
#if defined(__vita__)
 sceIoDclose(d);
#else
 closedir(d);
#endif
}
void psp_media_scan(Library*lib,const char*root){walk(lib,root,0);}
bool psp_native_installed(const char*id){
 if(!launcher_title_id_valid(id))return false;
 char path[128];snprintf(path,sizeof(path),"ux0:pspemu/PSP/GAME/%s/EBOOT.PBP",id);FILE*f=fopen(path,"rb");if(!f)return false;fclose(f);snprintf(path,sizeof(path),"ur0:appmeta/%s/icon0.dds",id);f=fopen(path,"rb");if(!f)return false;fclose(f);return true;
}
#if defined(__vita__)
typedef struct sqlite3 sqlite3;typedef struct sqlite3_stmt sqlite3_stmt;
extern int sqlite3_open_v2(const char*,sqlite3**,int,const char*);extern int sqlite3_close(sqlite3*);extern int sqlite3_prepare_v2(sqlite3*,const char*,int,sqlite3_stmt**,const char**);extern int sqlite3_step(sqlite3_stmt*);extern const unsigned char*sqlite3_column_text(sqlite3_stmt*,int);extern int sqlite3_finalize(sqlite3_stmt*);
static void*sql_alloc(int n){return n>0?malloc((size_t)n):NULL;}
static void*sql_realloc(void*p,int n){return realloc(p,(size_t)n);}
void psp_livearea_scan(Library*lib){
 if(sceSysmoduleLoadModule(SCE_SYSMODULE_SQLITE)<0 && sceSysmoduleIsLoaded(SCE_SYSMODULE_SQLITE)<0)return;
 SceSqliteMallocMethods methods={sql_alloc,sql_realloc,free};sceSqliteConfigMallocMethods(&methods);
 sqlite3*db=NULL;if(sqlite3_open_v2("ur0:shell/db/app.db",&db,1,NULL)){if(db)sqlite3_close(db);return;}
 const char*query="SELECT i.titleId,i.title,c.val,p.val FROM tbl_appinfo_icon i JOIN tbl_appinfo c ON c.titleId=i.titleId AND c.key=566916785 JOIN tbl_appinfo p ON p.titleId=i.titleId AND p.key=2593862978 WHERE c.val IN ('ME','EG','MG','UG')";
 sqlite3_stmt*st=NULL;if(!sqlite3_prepare_v2(db,query,-1,&st,NULL))while(sqlite3_step(st)==100){
  const char*id=(const char*)sqlite3_column_text(st,0),*title=(const char*)sqlite3_column_text(st,1),*cat=(const char*)sqlite3_column_text(st,2),*base=(const char*)sqlite3_column_text(st,3);if(!id || !base || !launcher_title_id_valid(id))continue;
  AppItem item;memset(&item,0,sizeof(item));snprintf(item.title_id,sizeof(item.title_id),"%s",id);snprintf(item.media_path,sizeof(item.media_path),"%s/EBOOT.PBP",base);if(!psp_read_metadata(item.media_path,&item))continue;if(title && *title)snprintf(item.title,sizeof(item.title),"%s",title);snprintf(item.category_str,sizeof(item.category_str),"%s",cat?cat:"");item.section=SECTION_PSP_PS1;item.exists=true;library_add_app(lib,&item);
 }if(st)sqlite3_finalize(st);
 st=NULL;if(!sqlite3_prepare_v2(db,"SELECT titleId,val FROM tbl_appinfo WHERE key=414472871",-1,&st,NULL))while(sqlite3_step(st)==100){const char*id=(const char*)sqlite3_column_text(st,0),*date=(const char*)sqlite3_column_text(st,1);int y,m,d,h,n,s;if(!id || !date || sscanf(date,"%d-%d-%d %d:%d:%d",&y,&m,&d,&h,&n,&s)!=6)continue;uint64_t installed=library_date(y,m,d,h,n,s);if(installed)for(int i=0;i<lib->count;i++)if(!strcmp(lib->items[i].title_id,id))lib->items[i].installed_at=installed;}
 if(st)sqlite3_finalize(st);
 sqlite3_close(db);
}
#else
void psp_livearea_scan(Library*lib){(void)lib;}
int psp_prepare_runtime(char*message,size_t capacity){snprintf(message,capacity,"Componente PSP requer a consola.");return -1;}
bool psp_create_bubble(const AppItem*item,char*message,size_t capacity){(void)item;snprintf(message,capacity,"Criar bolhas requer a PS Vita.");return false;}
#endif

void psp_identify_bubble(AppItem*item,const char*root){
 if(!strcmp(item->category_str,"ME")){item->type=APP_TYPE_PS1;item->section=SECTION_PSP_PS1;}
 else if(!strcmp(item->category_str,"EG") || !strcmp(item->category_str,"MG") || !strcmp(item->category_str,"UG")){item->type=APP_TYPE_PSP;item->section=SECTION_PSP_PS1;}
 char path[256];int n=snprintf(path,sizeof(path),"%s/%s/data/boot.bin",root,item->title_id);if(n<0 || n>=(int)sizeof(path))return;
 FILE*f=fopen(path,"rb");if(!f)return;unsigned char b[320];size_t size=fread(b,1,sizeof(b),f);fclose(f);
 if(size<288 || le(b)!=0x00424241)return;
 unsigned off=size>=320?64:32;if(!memchr(b+off,0,size-off))return;
 AppItem media;memset(&media,0,sizeof(media));snprintf(media.title_id,sizeof(media.title_id),"%s",item->title_id);
 if(psp_read_metadata((const char*)b+off,&media)){item->type=media.type;item->section=SECTION_PSP_PS1;if(media.icon_path[0])snprintf(item->icon_path,sizeof(item->icon_path),"%s",media.icon_path);snprintf(item->media_path,sizeof(item->media_path),"%s",b+off);snprintf(item->game_id,sizeof(item->game_id),"%s",media.game_id);}
}
