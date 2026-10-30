#include "theme.h"
static int environment,text_choice;static bool contrast=true;
void theme_configure(int e,int t,bool high){environment=e>=0 && e<VS_THEME_COUNT?e:0;text_choice=t>=0 && t<5?t:0;contrast=high;}
const char*theme_environment_name(int i){const char*names[]={"Azul PlayStation","Grafite","Violeta","Verde profundo","Vermelho escuro","PS5 | Midnight","PS5 | Ice","PS5 | Cosmic","PS5 | Aurora","PS4 | Blue Wave","PS4 | Sunset","PS4 | Ocean","Aniversario | Classic","Aniversario | Platinum","Aniversario | Legacy"};return names[i>=0 && i<VS_THEME_COUNT?i:0];}
const char*theme_text_name(int i){const char*names[]={"Branco","Azul gelo","Marfim","Menta","Rosa claro"};return names[i>=0 && i<5?i:0];}
uint32_t theme_background(uint32_t color){
 const uint32_t known[]={0x0D0F12,0x0D172B,0x0D1729,0x111827,0x0F1B2E,0x15253B,0x152742,0x1F3D60,0x29405C,0x10243D,0x26344B,0x17273E,0x385A88,0x26364D};
 if(!environment)return color;
 uint32_t rgb=color&0xffffff;bool match=false;for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);i++)if(rgb==known[i])match=true;if(!match)return color;
 unsigned r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255,l=(r+g+b)/3;
 const unsigned tint[VS_THEME_COUNT][3]={{0,0,0},{100,108,120},{122,76,160},{45,135,108},{150,65,77},{70,90,140},{85,135,160},{115,65,160},{55,145,125},{45,85,180},{170,80,90},{35,130,175},{120,120,118},{130,130,140},{120,110,100}};
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

uint32_t theme_canvas_color(int environment,int y){
 static const uint32_t top[VS_THEME_COUNT]={0x07101D,0x0B0D11,0x170B24,0x061D18,0x200A10,0x050A18,0x102B3B,0x180D2C,0x05231F,0x021A54,0x2C1134,0x032432,0x242728,0x171C28,0x282016};
 static const uint32_t bottom[VS_THEME_COUNT]={0x12375D,0x252A32,0x37214D,0x164937,0x4A1923,0x162752,0x32586B,0x412467,0x0C5648,0x0A4CA0,0x69283D,0x096879,0x555956,0x444E61,0x5A4938};
 if(environment<0 || environment>=VS_THEME_COUNT)environment=0;
 if(y<0)y=0;
 if(y>544)y=544;
 uint32_t a=top[environment],b=bottom[environment],out=0xFF000000;
 for(int shift=0;shift<=16;shift+=8){int x=(a>>shift)&255,z=(b>>shift)&255;out|=(uint32_t)(x+(z-x)*y/544)<<shift;}
 return out;
}
