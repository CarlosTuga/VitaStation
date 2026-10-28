#include "theme.h"
static int environment,text_choice;static bool contrast=true;
void theme_configure(int e,int t,bool high){environment=e>=0 && e<5?e:0;text_choice=t>=0 && t<5?t:0;contrast=high;}
const char*theme_environment_name(int i){const char*names[]={"Azul PlayStation","Grafite","Violeta","Verde profundo","Vermelho escuro"};return names[i>=0 && i<5?i:0];}
const char*theme_text_name(int i){const char*names[]={"Branco","Azul gelo","Marfim","Menta","Rosa claro"};return names[i>=0 && i<5?i:0];}
uint32_t theme_background(uint32_t color){
 const uint32_t known[]={0x0D0F12,0x0D172B,0x0D1729,0x111827,0x0F1B2E,0x15253B,0x152742,0x1F3D60,0x29405C,0x10243D,0x26344B,0x17273E,0x385A88,0x26364D};
 if(!environment)return color;
 uint32_t rgb=color&0xffffff;bool match=false;for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);i++)if(rgb==known[i])match=true;if(!match)return color;
 unsigned r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255,l=(r+g+b)/3;
 const unsigned tint[5][3]={{0,0,0},{100,108,120},{122,76,160},{45,135,108},{150,65,77}};
 unsigned rr=l*tint[environment][0]/100,gg=l*tint[environment][1]/100,bb=l*tint[environment][2]/100;if(rr>110)rr=110;if(gg>110)gg=110;if(bb>110)bb=110;
 return (color&0xff000000)|(rr<<16)|(gg<<8)|bb;
}
uint32_t theme_text(uint32_t color){
 unsigned r=(color>>16)&255,g=(color>>8)&255,b=color&255;
 if(r<100 && g<100 && b<100)return color; /* Selected text on a light button. */
 if(r>110 && g>110 && b>110){const uint32_t choices[]={0xF3F6FC,0xC7E9FF,0xFFF3D0,0xCCFFE5,0xFFDAEB};color=(color&0xff000000)|choices[text_choice];}
 if(contrast)color|=0xff000000;else if((color>>24)<0xd0)color=(color&0xffffff)|0xd0000000;
 return color;
}
bool theme_text_shadow(uint32_t color){return contrast && (((color>>16)&255)>100 || ((color>>8)&255)>100 || (color&255)>100);}
