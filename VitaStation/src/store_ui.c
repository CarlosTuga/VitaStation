#include "store_ui.h"
#include "store.h"
#include "render.h"
#include <stdio.h>
#include <string.h>
static bool visible(const UIContext*u){return u && u->catalog && (u->state==UI_STATE_CATALOG_DETAILS || (u->state==UI_STATE_HOME && u->current_section==SECTION_STORE));}
/* Keep parser enum IDs stable while removing Demos from navigation/loading. */
static const CatalogType store_types[]={CATALOG_GAMES,CATALOG_DLCS,CATALOG_THEMES,CATALOG_PSP,CATALOG_PSP_DLCS,CATALOG_PS1,CATALOG_PSM,CATALOG_HOMEBREW};
#define STORE_VISIBLE_COUNT (sizeof(store_types)/sizeof(store_types[0]))
#define STORE_TAB_WIDTH (880/(int)STORE_VISIBLE_COUNT)
static void change_type(UIContext*u,int step){
    int current=0;for(size_t i=0;i<STORE_VISIBLE_COUNT;i++)if(store_types[i]==u->catalog_type)current=(int)i;
    u->catalog_type=store_types[(current+step+(int)STORE_VISIBLE_COUNT)%(int)STORE_VISIBLE_COUNT];u->catalog_index=0;u->catalog_load_requested=true;
}
static void enqueue(UIContext*u){
    CatalogPackage p;char error[256];
    bool valid;
    if(u->catalog_type==CATALOG_HOMEBREW && u->state==UI_STATE_CATALOG_DETAILS && u->homebrew_link_index<u->homebrew_link_count)valid=catalog_homebrew_package(u->catalog,u->catalog_index,&u->homebrew_links[u->homebrew_link_index],&p,error,sizeof(error));
    else valid=catalog_get_package(u->catalog,u->catalog_index,&p,error,sizeof(error));
    if(!valid){ui_show_toast(u,error);return;}
    p.auto_dlcs=u->auto_dlcs_enabled && p.type==CATALOG_GAMES && !p.vpk;
    if(!store_enqueue(&p,error,sizeof(error)))ui_show_toast(u,error);
    else ui_show_toast(u,(!p.vpk && p.type!=CATALOG_HOMEBREW)?"Pedido enviado ao LiveArea; consulta as notificacoes.":"Instalador VPK/ZIP local: mantem o VitaStation aberto.");
}
bool store_ui_input(UIContext*u,StoreUICommand command){
    if(!visible(u))return false;
    if(u->catalog_type==CATALOG_DEMOS){u->catalog_type=CATALOG_GAMES;u->catalog_index=0;u->catalog_load_requested=true;}
    size_t n=u->catalog->count;
    if(command==STORE_UI_CANCEL){if(u->state==UI_STATE_CATALOG_DETAILS){if(u->homebrew_info_mode)u->homebrew_info_mode=false;else u->state=UI_STATE_HOME;return true;}return false;}
    if(command==STORE_UI_INFO){
        if(u->state==UI_STATE_CATALOG_DETAILS && u->catalog_type==CATALOG_HOMEBREW){u->homebrew_info_mode=!u->homebrew_info_mode;u->homebrew_text_scroll=0;return true;}
        if(n){u->state=UI_STATE_CATALOG_DETAILS;u->homebrew_info_mode=false;u->homebrew_link_count=catalog_homebrew_links(u->catalog,u->catalog_index,u->homebrew_links,24);u->homebrew_link_index=0;for(size_t i=0;i<u->homebrew_link_count;i++)if(u->homebrew_links[i].recommended){u->homebrew_link_index=i;break;}}return true;
    }
    if(command==STORE_UI_REFRESH){store_retry_covers();if(u->catalog_type==CATALOG_HOMEBREW)store_refresh_homebrew(true);u->catalog_load_requested=true;return true;}
    if(u->catalog_load_requested)return true;
    if(command==STORE_UI_CONFIRM){if(u->homebrew_info_mode){u->homebrew_info_mode=false;return true;}if(u->catalog_tabs){u->catalog_tabs=false;return true;}enqueue(u);return true;}
    if(u->state==UI_STATE_CATALOG_DETAILS){
        if(u->catalog_type==CATALOG_HOMEBREW){
            if(u->homebrew_info_mode){if((command==STORE_UI_UP || command==STORE_UI_LEFT) && u->homebrew_text_scroll)u->homebrew_text_scroll--;if((command==STORE_UI_DOWN || command==STORE_UI_RIGHT) && u->homebrew_text_scroll<40)u->homebrew_text_scroll++;return true;}
            if((command==STORE_UI_LEFT || command==STORE_UI_UP) && u->homebrew_link_index)u->homebrew_link_index--;
            if((command==STORE_UI_RIGHT || command==STORE_UI_DOWN) && u->homebrew_link_index+1<u->homebrew_link_count)u->homebrew_link_index++;
        }return true;
    }
    if(command==STORE_UI_LEFT){if(u->catalog_tabs)change_type(u,-1);else if(u->catalog_index)u->catalog_index--;}
    if(command==STORE_UI_RIGHT){if(u->catalog_tabs)change_type(u,1);else if(u->catalog_index+1<n)u->catalog_index++;}
    if(command==STORE_UI_UP){if(u->catalog_index%10<5)u->catalog_tabs=true;else u->catalog_index-=5;}
    if(command==STORE_UI_DOWN){if(u->catalog_tabs)u->catalog_tabs=false;else if(u->catalog_index+5<n)u->catalog_index+=5;}
    return true;
}
bool store_ui_touch(UIContext*u,int x,int y){
    if(!visible(u))return false;
    if(u->state==UI_STATE_CATALOG_DETAILS){
        if(u->catalog_type==CATALOG_HOMEBREW){
            if(u->homebrew_info_mode){if(y>430)u->homebrew_info_mode=false;return true;}
            if(x>=280 && x<920 && y>=264 && y<444){size_t first=u->homebrew_link_index>4?u->homebrew_link_index-4:0,index=first+(size_t)((y-264)/36);if(index<u->homebrew_link_count){if(index==u->homebrew_link_index)enqueue(u);else u->homebrew_link_index=index;}return true;}
            if(y>=462 && y<503)enqueue(u);
            return y>=60;
        }
        if(y>=406 && y<458 && x>=50 && x<400)enqueue(u);
            return y>=60;
    }
    if(y>=106 && y<140 && x>=40 && x<920){size_t tab=(size_t)((x-40)/STORE_TAB_WIDTH);if(tab<STORE_VISIBLE_COUNT){u->catalog_type=store_types[tab];u->catalog_index=0;u->catalog_tabs=true;u->catalog_load_requested=true;}return true;}
    if(y>=158 && y<454 && x>=40 && x<920){int col=(x-40)/176,row=(y-158)/148;size_t index=(u->catalog_index/10)*10+(size_t)(row*5+col);if(index<u->catalog->count){if(index==u->catalog_index && !u->catalog_tabs)enqueue(u);else{u->catalog_index=index;u->catalog_tabs=false;}}return true;}
    return y>=60;
}
static void short_text(int x,int y,const char*name,size_t limit,float scale){char out[128];size_t n=strlen(name);if(n>=sizeof(out))n=sizeof(out)-1;if(n>limit)n=limit;while(n && (((unsigned char)name[n]&0xc0)==0x80))n--;memcpy(out,name,n);out[n]=0;if(strlen(name)>n && n+3<sizeof(out))strcat(out,"...");render_draw_text(x,y,COLOR_WHITE,scale,out);}
/* Small region flags drawn procedurally; independent of fonts and image downloads. */
static bool region_flag(int x,int y,const char*region){
 if(!strcmp(region,"US")){
  for(int row=0;row<13;row++)render_fill_rect(x,y+row,24,1,row%2?0xFFFFFFFF:0xFFB22234);
  render_fill_rect(x,y,10,7,0xFF3C3B6E);
  for(int row=0;row<3;row++)for(int col=0;col<4;col++)render_fill_rect(x+1+col*2,y+1+row*2,1,1,0xFFFFFFFF);
  return true;
 }
 if(!strcmp(region,"EU")){
  render_fill_rect(x,y,24,14,0xFF003399);
  static const unsigned char stars[12][2]={{12,2},{14,3},{16,4},{17,6},{16,8},{14,10},{12,11},{10,10},{8,8},{7,6},{8,4},{10,3}};
  for(int i=0;i<12;i++)render_fill_rect(x+stars[i][0],y+stars[i][1],1,1,0xFFFFCC00);
  return true;
 }return false;
}
static void cover(UIContext*u,TextureCache*cache,size_t index,int x,int y,int w,int h){const char*path=catalog_resolve_cover(u->catalog,index);Texture*t=*path?texture_cache_load(cache,path):NULL;if(t)render_draw_texture_cover(t,x,y,w,h,1);else{const CatalogItem*item=catalog_get(u->catalog,index);if(item){if(item->homebrew)store_request_image(u->catalog_type,item->cache_key,item->homebrew->icon_url);else store_request_cover(u->catalog_type,item->title_id);}render_fill_rect(x,y,w,h,0xFF233A57);render_draw_text(x+12,y+h/2,0xFFD5E2F5,.7f,"VitaStation");}}
static void wrapped(int x,int y,const char*text,unsigned width,unsigned skip,unsigned lines,float scale){
    unsigned row=0;size_t at=0,length=strlen(text);
    while(at<length && row<skip+lines){size_t n=length-at;if(n>width)n=width;while(n && (((unsigned char)text[at+n]&0xc0)==0x80))n--;if(!n)break;
        if(row>=skip){char line[128];memcpy(line,text+at,n);line[n]=0;render_draw_text(x,y+(int)(row-skip)*19,COLOR_WHITE,scale,line);}at+=n;row++;}
}
static const char*link_type(CatalogLinkFormat format){switch(format){case CATALOG_LINK_VPK:return "VPK";case CATALOG_LINK_APP_ZIP:return "ZIP / VPK";case CATALOG_LINK_DATA_ZIP:return "Dados ZIP";case CATALOG_LINK_DATA_FILE:return "Ficheiro de dados";case CATALOG_LINK_PLUGIN:return "Plugin";default:return "Informacao";}}
static void homebrew_details(UIContext*u,TextureCache*cache,const CatalogItem*item){
    const HomebrewMetadata*h=item->homebrew;
    if(u->homebrew_info_mode){char text[2300];snprintf(text,sizeof(text),"%s  REQUISITOS: %s",h->description,h->requirements);short_text(50,152,item->name,66,1.15f);wrapped(50,190,text,90,u->homebrew_text_scroll,15,.8f);render_draw_text(50,494,0xFFB9CBE4,.7f,"Cima / Baixo: ler    X / O: voltar as ligacoes");return;}
    cover(u,cache,u->catalog_index,50,131,190,190);short_text(280,157,item->name,58,1.15f);
    char meta[240];snprintf(meta,sizeof(meta),"Versao %.31s  |  Autor: %.95s",h->version,h->author);short_text(280,188,meta,87,.7f);
    wrapped(280,214,h->description,79,0,2,.67f);render_draw_text(50,350,0xFFB9CBE4,.75f,"Requisitos");wrapped(50,372,h->requirements,28,0,5,.65f);
    size_t first=u->homebrew_link_index>4?u->homebrew_link_index-4:0;
    for(size_t i=first;i<u->homebrew_link_count && i<first+5;i++){
        const CatalogLink*link=&u->homebrew_links[i];int y=264+(int)(i-first)*36;bool selected=i==u->homebrew_link_index;
        if(selected)render_fill_rounded_rect(276,y,640,32,8,0xFF314B70);
        short_text(286,y+23,link->name,48,.7f);render_draw_text(734,y+23,0xFFB9CBE4,.65f,link_type(link->format));
    }
    const CatalogLink*selected=u->homebrew_link_index<u->homebrew_link_count?&u->homebrew_links[u->homebrew_link_index]:NULL;
    render_fill_rounded_rect(280,462,635,39,19,0xFFF2F5FA);render_draw_text(305,489,0xFF152137,.85f,selected && selected->format==CATALOG_LINK_REFERENCE?"Ligacao informativa (sem instalacao)":"X  Descarregar e instalar a ligacao selecionada");
    if(!u->homebrew_link_count)render_draw_text(280,300,COLOR_WHITE,.9f,"Nao existem ligacoes disponiveis.");
}
void store_ui_render(UIContext*u,TextureCache*cache){
    render_fill_rect(0,60,960,440,0xFF0D172B);
    render_draw_text(40,94,COLOR_WHITE,1.45f,"PlayStation Store");
    render_draw_text(653,91,0xFFB7C7DF,.7f,"VitaStation | A tua biblioteca digital");
    StoreProgress progress;store_progress(&progress);
    if(progress.offline_covers)render_draw_text(40,516,0xFFB9CBE4,.65f,"Caratulas locais | Jogos: Europa e EUA");
    if(u->catalog_type==CATALOG_HOMEBREW && progress.homebrew_error[0])render_draw_text(40,516,0xFFFFC58B,.6f,progress.homebrew_error);
    if(progress.covers_failed && progress.cover_error[0])render_draw_text(40,498,0xFFFFC58B,.6f,progress.cover_error);
    if(u->state==UI_STATE_CATALOG_DETAILS){
        const CatalogItem*item=catalog_get(u->catalog,u->catalog_index);if(!item)return;
        if(item->homebrew){homebrew_details(u,cache,item);return;}
        cover(u,cache,u->catalog_index,50,130,210,250);
        region_flag(300,157,item->region);short_text(334,171,item->name,40,1.25f);render_draw_text(300,211,0xFFB9CBE4,.9f,catalog_type_name(u->catalog_type));
        char info[192];snprintf(info,sizeof(info),"%s  |  %s",item->title_id,item->region);render_draw_text(300,252,COLOR_WHITE,.9f,info);
        if(item->has_size)snprintf(info,sizeof(info),"Tamanho: %.1f MB",(double)item->size_bytes/1048576.0);else snprintf(info,sizeof(info),"Tamanho: nao indicado no catalogo");render_draw_text(300,288,0xFFB9CBE4,.9f,info);
        render_draw_text(300,329,0xFFB9CBE4,.8f,"PKG: descarga e instalacao geridas pelo LiveArea.");
        render_fill_rounded_rect(50,406,350,46,23,0xFFF2F4F8);render_draw_text(90,437,0xFF152137,1,"X  Descarregar e instalar");return;
    }
    for(size_t i=0;i<STORE_VISIBLE_COUNT;i++){bool sel=u->catalog_type==store_types[i];int x=40+(int)i*STORE_TAB_WIDTH;if(sel)render_fill_rounded_rect(x,106,STORE_TAB_WIDTH-5,31,15,u->catalog_tabs?0xFFF2F5FA:0xFF314B70);render_draw_text(x+9,128,sel && u->catalog_tabs?0xFF172138:COLOR_WHITE,.72f,catalog_type_name(store_types[i]));}
    if(u->catalog_load_requested){render_draw_text(50,250,COLOR_WHITE,1.1f,"A carregar o catalogo...");return;}
    if(u->catalog->status!=CATALOG_READY || !u->catalog->count){
        render_draw_text(50,232,COLOR_WHITE,1.1f,u->catalog->status==CATALOG_INVALID?"Nao foi possivel ler este catalogo.":"Ainda nao ha itens nesta categoria.");
        render_draw_text(50,277,0xFFB9CBE4,.8f,"Coloca o TSV em ux0:usagi-pkgj ou ux0:pkgj.");
        render_draw_text(50,312,0xFFB9CBE4,.8f,"O formato JSON e usado se nao existir um TSV.");return;
    }
    size_t start=u->catalog_index/10*10;
    for(size_t i=start;i<u->catalog->count && i<start+10;i++){
        int cell=(int)(i-start),x=40+(cell%5)*176,y=157+(cell/5)*148;bool sel=i==u->catalog_index && !u->catalog_tabs;
        render_fill_rounded_rect(x-3,y-3,164,138,7,sel?0xFFF8FAFF:0xFF26344B);cover(u,cache,i,x,y,158,102);
        render_fill_rect(x,y+102,158,29,sel?0xFF385A88:0xFF17273E);bool flag=region_flag(x+5,y+110,u->catalog->items[i].region);short_text(x+(flag?34:6),y+122,u->catalog->items[i].name,flag?18:22,.7f);
    }
    char page[128];snprintf(page,sizeof(page),"%zu de %zu  |  X: descarregar  |  Triangulo: informacao  |  Quadrado: atualizar",u->catalog_index+1,u->catalog->count);render_draw_text(40,478,0xFFB9CBE4,.7f,page);
    if(u->catalog_type==CATALOG_HOMEBREW && !progress.covers_failed)render_draw_text(40,498,0xFFB9CBE4,.6f,"Catalogo: PS Vita Alive Store e contribuidores | CC BY 4.0 (quando aplicavel)");
}
void store_ui_progress(void){
    StoreProgress p;store_progress(&p);if(!p.active && !p.queued)return;
    render_fill_rounded_rect(600,412,332,84,12,0xF20C1425);short_text(615,434,p.title,34,.75f);
    render_draw_text(615,455,COLOR_WHITE,.65f,p.phase==STORE_INSTALLING?"A instalar automaticamente...":"Transferencia local: manter launcher aberto");
    render_fill_rect(615,468,300,3,0xFF344967);int w=p.total?(int)(300.0*(double)p.downloaded/(double)p.total):0;if(w>300)w=300;if(w>0)render_fill_rect(615,468,w,3,0xFFF2F5FA);
    char text[64];snprintf(text,sizeof(text),"%u na fila | %.1f / %.1f MB",p.queued,(double)p.downloaded/1048576.0,(double)p.total/1048576.0);render_draw_text(615,487,0xFFB9CBE4,.6f,text);
}
