#include "personalization.h"
#include "settings.h"
#include "theme.h"
#include "render.h"
#include "launcher.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#if defined(__vita__)
#include <psp2/io/dirent.h>
#include <psp2/photoexport.h>
#include <psp2/sysmodule.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
typedef struct {char name[96],path[256];int value;} Asset;
static Asset assets[128];static int asset_count;
static const char*menus[]={"Tema do VitaStation","Fonte das letras","Wallpaper personalizado","Exportar imagem de bloqueio","Abrir Fotografias para aplicar"};
static const char*base_name(const char*p){const char*s=strrchr(p,'/');return s?s+1:p;}
static int extension(const char*p,const char*ext){const char*s=strrchr(p,'.');if(!s)return 0;while(*s && *ext)if(tolower((unsigned char)*s++)!=*ext++)return 0;return !*s && !*ext;}
static void add_asset(const char*name,const char*path,int value){if(asset_count>=128 || strlen(path)>=256)return;Asset*a=&assets[asset_count++];snprintf(a->name,sizeof(a->name),"%.95s",name);snprintf(a->path,sizeof(a->path),"%s",path);a->value=value;}
static void scan_assets(const char*root,int kind,int depth){
 if(depth>2 || asset_count>=128)return;
#if defined(__vita__)
 SceUID d=sceIoDopen(root);if(d<0)return;SceIoDirent e;
 while(asset_count<128){memset(&e,0,sizeof(e));if(sceIoDread(d,&e)<=0)break;const char*name=e.d_name;bool directory=SCE_S_ISDIR(e.d_stat.st_mode);
#else
 DIR*d=opendir(root);if(!d)return;struct dirent*e;
 while(asset_count<128 && (e=readdir(d))){const char*name=e->d_name;char test[512];snprintf(test,sizeof(test),"%s/%s",root,name);struct stat st;bool directory=!stat(test,&st) && S_ISDIR(st.st_mode);
#endif
  if(name[0]=='.')continue;
  char path[512];int n=snprintf(path,sizeof(path),"%s/%s",root,name);if(n<0 || n>=256)continue;
  if(directory){scan_assets(path,kind,depth+1);continue;}
  bool wanted=kind==1?extension(path,".ttf"):(extension(path,".png") || ((kind==2 || kind==3) && (extension(path,".jpg") || extension(path,".jpeg"))));
  if(wanted)add_asset(name,path,0);
 }
#if defined(__vita__)
 sceIoDclose(d);
#else
 closedir(d);
#endif
}
static void picker(UIContext*u){
 u->asset_kind=u->personal_index;u->asset_index=0;asset_count=0;
 settings_ensure_dir(VS_DATA_DIR "/wallpapers");settings_ensure_dir(VS_DATA_DIR "/lockscreens");settings_ensure_dir(VS_DATA_DIR "/fonts");
 if(!u->asset_kind){for(int i=5;i<VS_THEME_COUNT;i++)add_asset(theme_environment_name(i),"",i);for(int i=0;i<5;i++)add_asset(theme_environment_name(i),"",i);}
 else if(u->asset_kind==1){add_asset("Sistema Vita (original)","",0);add_asset("Rajdhani | moderno","app0:assets/fonts/Rajdhani-Medium.ttf",0);add_asset("Audiowide | aniversario","app0:assets/fonts/Audiowide-Regular.ttf",0);scan_assets(VS_DATA_DIR "/fonts",1,0);}
 else{add_asset(u->asset_kind==2?"Automatico | fundos dos jogos":"Remover selecao do VitaStation","",0);scan_assets(u->asset_kind==2?VS_DATA_DIR "/wallpapers":VS_DATA_DIR "/lockscreens",u->asset_kind,0);scan_assets("ux0:picture",u->asset_kind,0);}
 u->state=UI_STATE_ASSET_PICKER;
}
bool personalization_image_valid(const char*path,bool lockscreen){
 FILE*f=fopen(path,"rb");if(!f)return false;unsigned char h[33];size_t n=fread(h,1,sizeof(h),f);bool ok=false;
 if(n==sizeof(h) && !memcmp(h,"\x89PNG\r\n\x1a\n",8) && !memcmp(h+12,"IHDR",4)){
  unsigned w=((unsigned)h[16]<<24)|((unsigned)h[17]<<16)|(h[18]<<8)|h[19],v=((unsigned)h[20]<<24)|((unsigned)h[21]<<16)|(h[22]<<8)|h[23];
  ok=w && v && w<=1920 && v<=1088;
  (void)lockscreen;
 }else if(n>=2 && h[0]==255 && h[1]==216)ok=true; /* Decoder enforces JPEG dimensions. */
 if(fseek(f,0,SEEK_END) || ftell(f)<=0 || ftell(f)>8*1024*1024)ok=false;
 fclose(f);return ok;
}
static bool export_lock_picture(const char*source){
#if defined(__vita__)
 int code=sceSysmoduleLoadModule(SCE_SYSMODULE_PHOTO_EXPORT);if(code<0){vs_log("lock-photo","load module",code);return false;}
 static char work[64*1024];PhotoExportParam param;memset(&param,0,sizeof(param));param.version=0x03150021;
 param.photoTitle=(const SceWChar32*)"VitaStation - imagem de bloqueio";param.gameTitle=(const SceWChar32*)"VitaStation";char destination[256]={0};
 code=scePhotoExportFromFile(source,&param,work,NULL,NULL,destination,sizeof(destination));sceSysmoduleUnloadModule(SCE_SYSMODULE_PHOTO_EXPORT);
 vs_log("lock-photo","export image to native Photos library",code);return code>=0;
#else
 (void)source;return false;
#endif
}
bool personalization_input(UIContext*u,Library*lib,SystemUICommand command){
 (void)lib;if(u->state!=UI_STATE_PERSONALIZATION && u->state!=UI_STATE_ASSET_PICKER)return false;
 if(command==SYSTEM_UI_CANCEL){u->state=u->state==UI_STATE_ASSET_PICKER?UI_STATE_PERSONALIZATION:UI_STATE_SETTINGS;return true;}
 int*index=u->state==UI_STATE_PERSONALIZATION?&u->personal_index:&u->asset_index;int count=u->state==UI_STATE_PERSONALIZATION?5:asset_count;
 if((command==SYSTEM_UI_UP || command==SYSTEM_UI_LEFT) && *index>0)(*index)--;
 if((command==SYSTEM_UI_DOWN || command==SYSTEM_UI_RIGHT) && *index+1<count)(*index)++;
 if(command!=SYSTEM_UI_CONFIRM)return true;
 if(u->state==UI_STATE_PERSONALIZATION){if(u->personal_index==4){if(launcher_launch_app("NPXS10004"))u->exit_requested=launcher_exit_required();else ui_show_toast(u,launcher_error_message());return true;}picker(u);return true;}
 if(u->asset_index<0 || u->asset_index>=asset_count)return true;
 Asset*a=&assets[u->asset_index];
 if(!u->asset_kind){int before=u->environment_theme;u->environment_theme=a->value;if(!settings_save(u,VS_CONFIG_PATH)){u->environment_theme=before;ui_show_toast(u,"Nao foi possivel guardar o tema.");return true;}}
 else{
  if(u->asset_kind>=2 && a->path[0] && !personalization_image_valid(a->path,u->asset_kind==3)){ui_show_toast(u,u->asset_kind==3?"Imagem de bloqueio invalida ou demasiado grande.":"Imagem invalida ou demasiado grande.");return true;}
  if(u->asset_kind==3 && a->path[0] && !export_lock_picture(a->path)){ui_show_toast(u,"Nao foi possivel exportar a imagem para Fotografias.");return true;}
  char*dest=u->asset_kind==1?u->font_path:u->asset_kind==2?u->wallpaper_path:u->lock_path;char old[256];memcpy(old,dest,sizeof(old));
  if(u->asset_kind==1 && !render_set_font(a->path)){ui_show_toast(u,"Nao foi possivel carregar a fonte.");return true;}
  snprintf(dest,256,"%s",a->path);
  if(!personalization_save(u,VS_DATA_DIR "/appearance.ini")){memcpy(dest,old,256);if(u->asset_kind==1)render_set_font(old);ui_show_toast(u,"Nao foi possivel guardar a personalizacao.");return true;}
 }
 u->state=UI_STATE_PERSONALIZATION;ui_show_toast(u,u->asset_kind==3 && a->path[0]?"Imagem em Fotografias: escolhe Usar como ecra inicial.":"Personalizacao guardada.");return true;
}
bool personalization_touch(UIContext*u,Library*lib,int x,int y){
 if(u->state!=UI_STATE_PERSONALIZATION && u->state!=UI_STATE_ASSET_PICKER)return false;
 if(y<125 && x<90){personalization_input(u,lib,SYSTEM_UI_CANCEL);return true;}
 int*index=u->state==UI_STATE_PERSONALIZATION?&u->personal_index:&u->asset_index,count=u->state==UI_STATE_PERSONALIZATION?5:asset_count,first=*index>5?*index-5:0;
 if(x>=35 && x<605 && y>=145 && y<439){int row=first+(y-145)/42;if(row<count){*index=row;personalization_input(u,lib,SYSTEM_UI_CONFIRM);}}return true;
}
bool personalization_render(UIContext*u,TextureCache*cache){
 if(u->state!=UI_STATE_PERSONALIZATION && u->state!=UI_STATE_ASSET_PICKER)return false;
 render_fill_rect(0,60,960,444,0xFF111827);render_draw_text(40,112,COLOR_WHITE,1.2f,"Personalizacao");
 int selected=u->state==UI_STATE_PERSONALIZATION?u->personal_index:u->asset_index,count=u->state==UI_STATE_PERSONALIZATION?5:asset_count,first=selected>5?selected-5:0;
 for(int i=first;i<count && i<first+7;i++){int y=172+(i-first)*42;if(i==selected)render_fill_rounded_rect(30,y-28,570,37,9,0xFF315477);render_draw_text(45,y,COLOR_WHITE,.76f,u->state==UI_STATE_PERSONALIZATION?menus[i]:assets[i].name);}
 render_draw_text(625,160,COLOR_WHITE,.78f,"Tema atual");render_draw_text(625,194,COLOR_WHITE,.66f,theme_environment_name(u->environment_theme));
 if(u->state==UI_STATE_ASSET_PICKER && u->asset_kind==0 && selected<count){for(int y=0;y<175;y+=5)render_fill_rect(620,215+y,310,5,theme_canvas_color(assets[selected].value,y*544/175));render_draw_text(640,310,COLOR_WHITE,.65f,"VitaStation");}
 else if(u->state==UI_STATE_ASSET_PICKER && u->asset_kind>=2 && selected<count && assets[selected].path[0]){Texture*t=texture_cache_load(cache,assets[selected].path);if(t)render_draw_texture_cover(t,620,215,310,175,1);}
 else{render_draw_text(625,250,COLOR_WHITE,.65f,"Wallpaper:");char name[40];snprintf(name,sizeof(name),"%.36s",u->wallpaper_path[0]?base_name(u->wallpaper_path):"Fundos dos jogos");render_draw_text(625,280,COLOR_WHITE,.6f,name);}
 if(u->state==UI_STATE_PERSONALIZATION && u->personal_index>=3){render_draw_text(620,335,COLOR_WHITE,.62f,"1. Exportar a tua imagem");render_draw_text(620,365,COLOR_WHITE,.62f,"2. Abrir Fotografias");render_draw_text(620,395,COLOR_WHITE,.62f,"3. Usar como ecra inicial");}
 render_draw_text(40,465,COLOR_WHITE,.65f,"X: escolher | O: voltar | Imagens em data/VitaStation/wallpapers ou picture");
 if(u->state==UI_STATE_ASSET_PICKER && u->asset_kind==3)render_draw_text(40,494,COLOR_WHITE,.62f,"Bloqueio: em Fotografias, abre a imagem e escolhe Usar como ecra inicial.");
 return true;
}
void personalization_background(UIContext*u,TextureCache*cache){
 if(u->wallpaper_path[0]){Texture*t=texture_cache_load(cache,u->wallpaper_path);if(t){render_draw_texture_cover(t,0,0,960,544,1);return;}}
 for(int y=0;y<544;y+=8)render_fill_rect(0,y,960,8,theme_canvas_color(u->environment_theme,y));
 int style=u->environment_theme>=12?2:u->environment_theme>=9?1:0;
 if(style==2){for(int y=75;y<540;y+=32)for(int x=20;x<960;x+=32)render_fill_rect(x,y,2,2,0x346CD9CC);for(int i=0;i<4;i++)render_fill_rounded_rect(780+i*25,370-i*20,12,90,5,0x503B70D0+(unsigned)i*0x000D1700);}
 else for(int wave=0;wave<4;wave++)for(int x=0;x<960;x+=8){int y=300+(int)(45*sinf((float)x/220+wave*.4f))+wave*12;render_draw_line(x,y,x+8,300+(int)(45*sinf((float)(x+8)/220+wave*.4f))+wave*12,(style==1?0x386ABFFF:0x207CCAE4),style==1?2:1);}
}
