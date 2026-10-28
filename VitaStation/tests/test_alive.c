#include "catalog.h"
#include "ui.h"
#include "store_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static StoreCatalog catalog;
static UIContext ui;
static TextureCache cache;
int main(int argc,char**argv){
    assert(argc==2);catalog_init(&catalog);
    assert(catalog_load_root(&catalog,argv[1],CATALOG_HOMEBREW));
    assert(catalog.count==1150 && !catalog.truncated && catalog.source_json);
    mkdir("cover-fixture");char alias[128];snprintf(alias,sizeof(alias),"cover-fixture/%.19s.png",catalog.items[0].cache_key);
    FILE*image=fopen(alias,"wb");assert(image);const unsigned char fixture_png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,8,29,99,248,207,192,240,31,0,5,128,2,63,73,194,237,180,0,0,0,0,73,69,78,68,174,66,96,130};assert(fwrite(fixture_png,1,sizeof(fixture_png),image)==sizeof(fixture_png));fclose(image);
    assert(!strcmp(catalog_resolve_cover(&catalog,0),alias));
    char saved[256];snprintf(saved,sizeof(saved),"%s",alias);catalog_invalidate_covers(&catalog);assert(!strcmp(catalog_resolve_cover(&catalog,0),saved));
    mkdir("packed-fixture");char packed[128];snprintf(packed,sizeof(packed),"packed-fixture/%s.png",catalog.items[1].cache_key);
    image=fopen(packed,"wb");assert(image);assert(fwrite(fixture_png,1,sizeof(fixture_png),image)==sizeof(fixture_png));fclose(image);
    assert(!strcmp(catalog_resolve_cover(&catalog,1),packed));catalog_invalidate_covers(&catalog);assert(!strcmp(catalog_resolve_cover(&catalog,1),packed));
    unsigned noid=0;for(size_t i=0;i<catalog.count;i++){const CatalogItem*a=&catalog.items[i];assert(a->homebrew && a->homebrew->id[0] && a->cache_key[0]);if(!a->title_id[0])noid++;}
    assert(noid>=3);CatalogLink links[24];CatalogPackage package;char error[256];unsigned tested=0;
    const char*ids[]={"hollow-knight-vita","peelgallery","azahar-emulator-alpha","pacman-ce-dx-vita","ff4-vita","cuphead-vita"};
    for(size_t i=0;i<catalog.count;i++)for(unsigned k=0;k<sizeof(ids)/sizeof(*ids);k++)if(!strcmp(catalog.items[i].homebrew->id,ids[k])){
        size_t n=catalog_homebrew_links(&catalog,i,links,24);assert(n>0);assert(catalog_get_package(&catalog,i,&package,error,sizeof(error)) && package.vpk && package.cache_key[0]);
        for(size_t j=0;j<n;j++){
            bool valid=catalog_homebrew_package(&catalog,i,&links[j],&package,error,sizeof(error));
            if(links[j].format==CATALOG_LINK_REFERENCE)assert(!valid);
            if(links[j].format==CATALOG_LINK_PLUGIN)assert(valid && !package.vpk && !strcmp(package.plugin_section,"*KERNEL"));
        }tested++;
    }
    assert(tested==6);
    ui_init(&ui);ui.catalog=&catalog;ui.current_section=SECTION_STORE;ui.catalog_type=CATALOG_HOMEBREW;ui.catalog_index=0;ui.catalog_load_requested=false;
    assert(store_ui_input(&ui,STORE_UI_INFO) && ui.homebrew_link_count && ui.state==UI_STATE_CATALOG_DETAILS);
    assert(store_ui_input(&ui,STORE_UI_INFO) && ui.homebrew_info_mode);
    assert(store_ui_input(&ui,STORE_UI_DOWN) && ui.homebrew_text_scroll==1);
    assert(store_ui_input(&ui,STORE_UI_CANCEL) && !ui.homebrew_info_mode && ui.state==UI_STATE_CATALOG_DETAILS);
    texture_cache_init(&cache,8);store_ui_render(&ui,&cache);texture_cache_clear(&cache);
    assert(store_ui_input(&ui,STORE_UI_CANCEL) && ui.state==UI_STATE_HOME);
    const CatalogType expected[]={CATALOG_GAMES,CATALOG_DLCS,CATALOG_THEMES,CATALOG_PSP,CATALOG_PSP_DLCS,CATALOG_PS1,CATALOG_PSM,CATALOG_HOMEBREW};
    ui.catalog_tabs=true;ui.catalog_type=CATALOG_GAMES;
    for(unsigned i=0;i<16;i++){ui.catalog_load_requested=false;assert(store_ui_input(&ui,STORE_UI_RIGHT));assert(ui.catalog_type==expected[(i+1)%8] && ui.catalog_type!=CATALOG_DEMOS);}
    ui.catalog_load_requested=false;assert(store_ui_input(&ui,STORE_UI_LEFT) && ui.catalog_type==CATALOG_HOMEBREW);
    for(unsigned i=0;i<8;i++){assert(store_ui_touch(&ui,40+(int)i*110+55,120));assert(ui.catalog_type==expected[i] && ui.catalog_type!=CATALOG_DEMOS);}
    catalog_clear(&catalog);puts("Alive: 1150 entries, unknown IDs, typed links, references and details UI passed.");return 0;
}
