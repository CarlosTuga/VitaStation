#include "settings_menu.h"
#include "system_controls.h"
#include "system_info.h"
#include "render.h"
#include <string.h>
static bool available[512],availability_ready=false;
int settings_group_for(int index){
 if(index<0)return SETTINGS_ROOT;
 if(index<UI_LOCAL_SETTINGS_COUNT)return SETTINGS_VITASTATION;
 const SystemFeature*f=system_feature_get((size_t)(index-UI_LOCAL_SETTINGS_COUNT));if(!f || f->control<0)return SETTINGS_ROOT;
 if(availability_ready && (size_t)(index-UI_LOCAL_SETTINGS_COUNT)<512 && !available[index-UI_LOCAL_SETTINGS_COUNT])return SETTINGS_ROOT;
 if((f->control>=SYS_ACTION_LIBRARY && f->control!=SYS_ACTION_SUSPEND) || f->control==SYS_INFO_DOWNLOADS || f->control==SYS_INFO_LOG || f->control==SYS_INFO_LICENSES || !strcmp(f->category,"Integração do launcher") || !strcmp(f->category,"VitaStation"))return SETTINGS_VITASTATION;
 return SETTINGS_CONSOLE;
}
int settings_group_count(int group){int count=0;for(int i=0;i<UI_LOCAL_SETTINGS_COUNT+(int)system_feature_count();i++)if(settings_group_for(i)==group)count++;return count;}
int settings_group_item(int group,int position){for(int i=0;i<UI_LOCAL_SETTINGS_COUNT+(int)system_feature_count();i++)if(settings_group_for(i)==group && position--==0)return i;return -1;}
int settings_group_position(int group,int raw){int position=0;for(int i=0;i<UI_LOCAL_SETTINGS_COUNT+(int)system_feature_count();i++)if(settings_group_for(i)==group){if(i==raw)return position;position++;}return -1;}
static void refresh_available(void){
 availability_ready=false;for(size_t i=0;i<system_feature_count() && i<512;i++){
  const SystemFeature*f=system_feature_get(i);available[i]=f && f->control>=0;
#if defined(__vita__)
  if(available[i] && settings_group_for((int)i+UI_LOCAL_SETTINGS_COUNT)==SETTINGS_CONSOLE){
   if(f->control>=100 && f->control<200){char lines[6][128];available[i]=system_info_read(f->control,lines)>0;}
   else if(f->control==SYS_VOLUME || f->control==SYS_BRIGHTNESS){SystemControlResult r=system_control_read((SystemControl)f->control);available[i]=r.status==SYS_APPLIED || r.status==SYS_BRIGHTNESS_SAVED;}
  }
#endif
 }availability_ready=true;
}
void settings_menu_open(UIContext*u){refresh_available();u->state=UI_STATE_SETTINGS;u->settings_group=SETTINGS_ROOT;u->settings_root_index=0;}
bool settings_menu_input(UIContext*u,SystemUICommand command){
 if(u->state!=UI_STATE_SETTINGS)return false;
 if(u->settings_group==SETTINGS_ROOT){
  if(command==SYSTEM_UI_CANCEL){u->state=UI_STATE_HOME;return true;}
  if(command==SYSTEM_UI_LEFT || command==SYSTEM_UI_UP)u->settings_root_index=0;
  if(command==SYSTEM_UI_RIGHT || command==SYSTEM_UI_DOWN)u->settings_root_index=1;
  if(command==SYSTEM_UI_CONFIRM){u->settings_group=u->settings_root_index?SETTINGS_VITASTATION:SETTINGS_CONSOLE;u->settings_index=settings_group_item(u->settings_group,0);}
  return true;
 }
 if(command==SYSTEM_UI_CANCEL){u->settings_root_index=u->settings_group==SETTINGS_VITASTATION;u->settings_group=SETTINGS_ROOT;return true;}
 if(command==SYSTEM_UI_CONFIRM)return false;
 if((command==SYSTEM_UI_LEFT || command==SYSTEM_UI_RIGHT) && u->settings_index>=UI_LOCAL_SETTINGS_COUNT){
  const SystemFeature*f=system_feature_get((size_t)(u->settings_index-UI_LOCAL_SETTINGS_COUNT));if(f && f->control>=0 && f->control<=SYS_BRIGHTNESS)return false;
 }
 int pos=settings_group_position(u->settings_group,u->settings_index),count=settings_group_count(u->settings_group);
 if(pos<0)pos=0;
 if((command==SYSTEM_UI_LEFT || command==SYSTEM_UI_UP) && pos>0)pos--;
 if((command==SYSTEM_UI_RIGHT || command==SYSTEM_UI_DOWN) && pos+1<count)pos++;
 u->settings_index=settings_group_item(u->settings_group,pos);return true;
}
bool settings_menu_touch(UIContext*u,Library*lib,int x,int y){
 if(u->state!=UI_STATE_SETTINGS)return false;
 if(y>=72 && y<=113 && x<42){ui_cancel(u);return true;}
 if(u->settings_group==SETTINGS_ROOT){
  if(y>=165 && y<420 && x>=60 && x<900){u->settings_root_index=x>=480;ui_confirm(u,lib);}return true;
 }
 int pos=settings_group_position(u->settings_group,u->settings_index),first=pos>5?pos-5:0;
 if(x>=36 && x<410 && y>=137 && y<459){int raw=settings_group_item(u->settings_group,first+(y-137)/46);if(raw>=0){if(raw==u->settings_index)ui_confirm(u,lib);else u->settings_index=raw;}}
 else if(x>=465 && x<900 && y>=388 && y<439)ui_confirm(u,lib);
 return true;
}
void settings_menu_icon(int x,int y,int size,int group,uint32_t color){
 int unit=size/12;if(unit<1)unit=1;
 if(group==SETTINGS_CONSOLE){
  render_fill_rounded_rect(x,y+unit,size,unit*7,unit*2,color);
  render_fill_rounded_rect(x+unit,y+unit*2,size-unit*2,unit*5,unit,0xFF152742);
  render_fill_rect(x+unit*2,y+unit*4,unit*3,unit,color);render_fill_rect(x+unit*3,y+unit*3,unit,unit*3,color);
  render_fill_rounded_rect(x+unit*8,y+unit*3,unit,unit,unit/2,color);render_fill_rounded_rect(x+unit*9,y+unit*5,unit,unit,unit/2,color);
  render_fill_rect(x+unit*4,y+unit*9,unit*4,unit,color);
 }else{
  render_fill_rounded_rect(x,y,size,size,unit*3,color);
  render_fill_rounded_rect(x+unit,y+unit,size-unit*2,size-unit*2,unit*2,0xFF152742);
  render_draw_line(x+unit*3,y+unit*3,x+unit*6,y+unit*9,color,unit);
  render_draw_line(x+unit*6,y+unit*9,x+unit*9,y+unit*3,color,unit);
  render_fill_rect(x+unit*9,y+unit*7,unit,unit,color);
 }
}
void settings_menu_root(UIContext*u){
 render_fill_rect(0,60,960,444,0xFF0D1729);
 render_fill_rounded_rect(640,70,290,400,90,0xFF10243D);
 render_draw_text(60,110,COLOR_WHITE,1.65f,"Definições");
 render_draw_text(60,142,0xFFB1C5DD,.8f,"Escolhe o que pretendes personalizar");
 const char*names[]={"Definições da consola","Definições do VitaStation"};
 const char*descriptions[]={"Sistema, rede e dispositivos","Biblioteca, loja e personalização"};
 for(int i=0;i<2;i++){
  int x=60+i*430;bool selected=i==u->settings_root_index;
  render_fill_rounded_rect(x-2,174,404,246,24,selected?0xFFEFF5FE:0xFF29405C);
  render_fill_rounded_rect(x,176,400,242,22,selected?0xFF1F3D60:0xFF15253B);
  render_fill_rounded_rect(x+25,198,112,104,22,0xFF152742);
  settings_menu_icon(x+47,220,66,i+1,selected?0xFFBCE4FF:0xFF8AAAC7);
  render_draw_text(x+25,342,COLOR_WHITE,1.04f,names[i]);
  render_draw_text(x+25,375,0xFFB8CDE3,.76f,descriptions[i]);
 }
 render_draw_text(60,469,0xFFB1C5DD,.8f,"L/R ou setas: escolher     X: abrir     O: voltar");
}

void settings_option_icon(int x,int y,int size,int raw,uint32_t color){
 enum {GEAR,NET,PERSON,TROPHY,BT,SOUND,DISPLAY,CLOCK,ACCESS,POWER,STORAGE,LIBRARY,STORE,DOWNLOAD,PICTURE,CONTROL,PLUGIN,REFRESH,PALETTE,TEXT,CONTRAST};
 int kind=GEAR;
 if(raw<UI_LOCAL_SETTINGS_COUNT){const int local[]={SOUND,STORE,PICTURE,LIBRARY,POWER,PLUGIN,CONTROL,POWER,DOWNLOAD,PALETTE,TEXT,CONTRAST,GEAR,DOWNLOAD,STORAGE,LIBRARY,PLUGIN,PERSON};if(raw>=0)kind=local[raw];}
 else{const SystemFeature*f=system_feature_get((size_t)(raw-UI_LOCAL_SETTINGS_COUNT));if(f){
  if(f->control==SYS_ACTION_REFRESH)kind=REFRESH;
  else if(f->control==SYS_ACTION_PLUGINS || f->control==SYS_ACTION_PLUGIN_PREPARE)kind=PLUGIN;
  else if(f->control==SYS_ACTION_POWER || f->control==SYS_ACTION_AUTOBOOT || f->control==SYS_ACTION_SUSPEND)kind=POWER;
  else if(f->control==SYS_INFO_DOWNLOADS)kind=DOWNLOAD;
  else if(f->control==SYS_ACTION_LIBRARY)kind=LIBRARY;
  else if(f->control==SYS_ACTION_STORE)kind=STORE;
  else if(f->control==SYS_ACTION_PS)kind=CONTROL;
  else if(f->control==SYS_VOLUME)kind=SOUND;
  else if(f->control==SYS_BRIGHTNESS)kind=DISPLAY;
  else if(!strcmp(f->category,"Rede"))kind=NET;
  else if(!strcmp(f->category,"Conta e PSN"))kind=PERSON;
  else if(!strcmp(f->category,"Troféus"))kind=TROPHY;
  else if(!strcmp(f->category,"Dispositivos"))kind=BT;
  else if(!strcmp(f->category,"Som e ecrã"))kind=SOUND;
  else if(!strcmp(f->category,"Notificações"))kind=SOUND;
  else if(!strcmp(f->category,"Idioma e relógio"))kind=CLOCK;
  else if(!strcmp(f->category,"Acessibilidade"))kind=ACCESS;
  else if(!strcmp(f->category,"Energia"))kind=POWER;
  else if(!strcmp(f->category,"Segurança e armazenamento"))kind=STORAGE;
 }}
 int u=size/12;if(u<1)u=1;int thick=u>1?2:1;
#define LINE(a,b,c,d) render_draw_line(x+(a)*u,y+(b)*u,x+(c)*u,y+(d)*u,color,thick)
#define BOX(a,b,c,d) render_draw_rect(x+(a)*u,y+(b)*u,(c)*u,(d)*u,color,thick)
#define FILL(a,b,c,d) render_fill_rect(x+(a)*u,y+(b)*u,(c)*u,(d)*u,color)
 if(kind==NET){LINE(1,4,3,2);LINE(3,2,8,2);LINE(8,2,10,4);LINE(3,6,5,4);LINE(5,4,7,6);FILL(5,8,2,2);}
 else if(kind==PERSON){render_fill_rounded_rect(x+4*u,y+u,4*u,4*u,2*u,color);LINE(1,10,2,7);LINE(2,7,9,7);LINE(9,7,10,10);LINE(1,10,10,10);}
 else if(kind==TROPHY){BOX(3,1,6,5);LINE(3,2,1,2);LINE(1,2,1,5);LINE(1,5,3,6);LINE(9,2,11,2);LINE(11,2,11,5);LINE(11,5,9,6);LINE(6,6,6,10);LINE(3,10,9,10);}
 else if(kind==BT){LINE(5,0,5,11);LINE(5,0,9,3);LINE(9,3,2,9);LINE(2,2,9,8);LINE(9,8,5,11);}
 else if(kind==SOUND){BOX(1,4,3,4);LINE(4,4,7,1);LINE(7,1,7,10);LINE(7,10,4,7);LINE(9,3,11,5);LINE(11,5,11,7);LINE(11,7,9,9);}
 else if(kind==DISPLAY || kind==LIBRARY || kind==STORAGE){BOX(1,2,10,7);LINE(4,11,8,11);if(kind==LIBRARY){LINE(4,3,4,8);LINE(7,3,7,8);}if(kind==STORAGE){FILL(3,4,1,1);LINE(6,4,9,4);LINE(3,7,9,7);}}
 else if(kind==CLOCK){render_draw_rect(x+u,y+u,10*u,10*u,color,thick);LINE(6,2,6,6);LINE(6,6,9,7);}
 else if(kind==POWER){LINE(6,0,6,5);LINE(3,2,1,4);LINE(1,4,1,8);LINE(1,8,4,11);LINE(4,11,8,11);LINE(8,11,11,8);LINE(11,8,11,4);LINE(11,4,9,2);}
 else if(kind==DOWNLOAD){LINE(6,1,6,8);LINE(3,5,6,8);LINE(6,8,9,5);LINE(1,9,1,11);LINE(1,11,11,11);LINE(11,11,11,9);}
 else if(kind==STORE){BOX(2,4,8,7);LINE(4,4,4,1);LINE(4,1,8,1);LINE(8,1,8,4);}
 else if(kind==PLUGIN){BOX(3,4,6,5);LINE(4,1,4,4);LINE(8,1,8,4);LINE(6,9,6,11);}
 else if(kind==REFRESH){LINE(2,3,4,1);LINE(4,1,9,1);LINE(9,1,11,4);LINE(11,4,8,4);LINE(10,8,8,10);LINE(8,10,3,10);LINE(3,10,1,7);LINE(1,7,4,7);}
 else if(kind==TEXT){LINE(1,11,5,1);LINE(5,1,10,11);LINE(3,7,8,7);}
 else if(kind==PALETTE){BOX(1,1,10,10);FILL(3,3,2,2);FILL(7,3,2,2);FILL(3,7,2,2);FILL(7,7,2,2);}
 else if(kind==CONTRAST){BOX(1,1,10,10);FILL(2,2,4,8);}
 else if(kind==PICTURE){BOX(1,1,10,10);LINE(2,9,5,5);LINE(5,5,7,8);LINE(7,8,9,6);FILL(8,3,1,1);}
 else if(kind==ACCESS){FILL(5,1,2,2);LINE(1,4,11,4);LINE(6,4,6,7);LINE(6,7,2,11);LINE(6,7,10,11);}
 else if(kind==CONTROL){settings_menu_icon(x,y,size,SETTINGS_CONSOLE,color);}
 else{BOX(3,3,6,6);LINE(6,0,6,3);LINE(6,9,6,12);LINE(0,6,3,6);LINE(9,6,12,6);FILL(5,5,2,2);}
#undef LINE
#undef BOX
#undef FILL
}
