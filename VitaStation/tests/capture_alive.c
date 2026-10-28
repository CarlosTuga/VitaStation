/* Host layout capture with real Alive metadata; textures are placeholders. */
#include "ui.h"
#include "store_ui.h"
#include <assert.h>
#include <stdlib.h>
static UIContext ui;static StoreCatalog catalog;static TextureCache cache;
int main(int argc,char**argv){assert(argc>1);catalog_init(&catalog);assert(catalog_load_root(&catalog,argv[1],CATALOG_HOMEBREW));ui_init(&ui);texture_cache_init(&cache,24);ui.catalog=&catalog;ui.catalog_type=CATALOG_HOMEBREW;ui.catalog_load_requested=false;ui.current_section=SECTION_STORE;ui.catalog_index=0;if(argc>2 && atoi(argv[2]))store_ui_input(&ui,STORE_UI_INFO);store_ui_render(&ui,&cache);texture_cache_clear(&cache);catalog_clear(&catalog);return 0;}
