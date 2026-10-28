#include "catalog.h"
#include "cover_pack.h"
#include "launcher.h"
#include "log.h"
#include "../third_party/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#define CATALOG_MAX_ITEMS 32768u
#define CATALOG_MAX_FILE (16u*1024u*1024u)
static const char *files[]={"titles_psvgames","titles_psvdlcs","titles_psvthemes","titles_psvdemos","titles_pspgames","titles_pspdlcs","titles_psxgames","titles_psmgames","titles_homebrew"};
static const char *names[]={"Jogos","DLC","Temas","Demos","PSP","PSP DLC","PS1","PSM","Homebrew"};
static const char *json_keys[]={"games","dlcs","themes","demos","psp_games","psp_dlcs","ps1_games","psm_games","homebrew"};
void catalog_init(StoreCatalog*c){if(c)memset(c,0,sizeof(*c));}
void catalog_clear(StoreCatalog*c){if(c){for(size_t i=0;i<c->count;i++)free(c->items[i].homebrew);free(c->items);catalog_init(c);}}
const char *catalog_type_name(CatalogType type){return (unsigned)type<CATALOG_TYPE_COUNT?names[type]:"Catalogo";}
const CatalogItem *catalog_get(const StoreCatalog*c,size_t i){return c && i<c->count?&c->items[i]:NULL;}
static bool path_join(char*out,size_t n,const char*a,const char*b){
    int written=snprintf(out,n,"%s%s%s",a,*a && a[strlen(a)-1]!='/'?"/":"",b);return written>=0 && (size_t)written<n;
}
static bool exists(const char *path){FILE*f=fopen(path,"rb");if(!f)return false;return fclose(f)==0;}
static bool image_exists(const char*path){
    unsigned char bytes[8];FILE*f=fopen(path,"rb");if(!f)return false;size_t n=fread(bytes,1,sizeof(bytes),f);fclose(f);
    return (n==8 && !memcmp(bytes,"\x89PNG\r\n\x1a\n",8)) || (n>=3 && bytes[0]==0xff && bytes[1]==0xd8 && bytes[2]==0xff);
}
static void text(char*out,size_t cap,const char*in){
    if(!in)in="";
    size_t len=strlen(in);if(len>=cap){len=cap-1;while(len && ((unsigned char)in[len]&0xC0)==0x80)len--;}
    memcpy(out,in,len);out[len]=0;for(size_t i=0;i<len;i++)if((unsigned char)out[i]<32)out[i]=' ';
}
static bool local_path(const char *path){return path && *path && !strstr(path,"://") && !strstr(path,"..") && strlen(path)<256;}
static void configure_covers(StoreCatalog*c){
    path_join(c->cover_dir,sizeof(c->cover_dir),c->root,"cover");
    char config[256];if(!path_join(config,sizeof(config),c->root,"config.txt"))return;
    FILE*f=fopen(config,"rb");if(!f)return;
    char line[512];while(fgets(line,sizeof(line),f)){
        char key[64],value[256];if(sscanf(line,"%63s %255[^\r\n]",key,value)==2 && !strcmp(key,"thumbnail_folder") && local_path(value))text(c->cover_dir,sizeof(c->cover_dir),value);
    }fclose(f);
}
static void find_cover(StoreCatalog*c,CatalogItem*item,const char*provided){
    char own[256];const char*key=item->cache_key[0]?item->cache_key:item->title_id;
    const char*packed_suffix[]={".png",".cover.jpg",".jpg"};
    for(unsigned i=0;i<3;i++){char name[128];snprintf(name,sizeof(name),"%s%s",key,packed_suffix[i]);if(cover_pack_find(name,item->cover,sizeof(item->cover)))return;}
    if(item->homebrew && !strncmp(key,"hb_",3) && strlen(key)>=19){for(unsigned i=0;i<3;i++){char name[128];snprintf(name,sizeof(name),"%.19s%s",key,packed_suffix[i]);if(cover_pack_find(name,item->cover,sizeof(item->cover)))return;}}
    const char*roots[]={VS_STORE_COVERS_DIR,VS_PACKED_COVERS_DIR};
    const char*own_suffix[]={".png",".cover.jpg",".jpg"};
    for(unsigned root=0;root<sizeof(roots)/sizeof(roots[0]);root++){
        for(unsigned i=0;i<3;i++){
            snprintf(own,sizeof(own),"%s/%s%s",roots[root],key,own_suffix[i]);
            if(image_exists(own)){text(item->cover,sizeof(item->cover),own);return;}
        }
        /* Stable alias also works for images installed inside this VPK. */
        if(item->homebrew && !strncmp(key,"hb_",3) && strlen(key)>=19){
            for(unsigned i=0;i<3;i++){
                snprintf(own,sizeof(own),"%s/%.19s%s",roots[root],key,own_suffix[i]);
                if(image_exists(own)){text(item->cover,sizeof(item->cover),own);return;}
            }
        }
    }
    if(provided && local_path(provided)){
        if(strchr(provided,':') || provided[0]=='/'){if(image_exists(provided)){text(item->cover,sizeof(item->cover),provided);return;}}
        else if(path_join(item->cover,sizeof(item->cover),c->cover_dir,provided) && image_exists(item->cover))return;
    }
    const char *suffix[]={".png",".cover.jpg",".jpg"};
    for(unsigned i=0;i<3;i++){
        char file[48];snprintf(file,sizeof(file),"%s%s",item->title_id,suffix[i]);
        if(path_join(item->cover,sizeof(item->cover),c->cover_dir,file) && image_exists(item->cover))return;
    }item->cover[0]=0;
}
static const char*store_region(const char*region){
    char token[24];size_t n=0;while(*region && isspace((unsigned char)*region))region++;
    while(*region && isalpha((unsigned char)*region) && n<sizeof(token)-1)token[n++]=(char)toupper((unsigned char)*region++);
    token[n]=0;
    if(!strcmp(token,"EU") || !strcmp(token,"EUR") || !strcmp(token,"EUROPE") || !strcmp(token,"EUROPEAN"))return "EU";
    if(!strcmp(token,"US") || !strcmp(token,"USA") || !strcmp(token,"NA") || !strcmp(token,"AMERICA"))return "US";
    return NULL;
}
static bool append(StoreCatalog*c,CatalogItem*item,const char *provided){
    if(c->type!=CATALOG_HOMEBREW){const char*region=store_region(item->region);if(!region){c->skipped++;free(item->homebrew);return true;}text(item->region,sizeof(item->region),region);}
    if(c->title_filter[0] && strcmp(c->title_filter,item->title_id)){free(item->homebrew);return true;}
    if((!launcher_title_id_valid(item->title_id) && !(c->type==CATALOG_HOMEBREW && item->homebrew && item->homebrew->id[0])) || !item->name[0]){c->skipped++;free(item->homebrew);return true;}
    if(!item->cache_key[0])text(item->cache_key,sizeof(item->cache_key),item->title_id);
    if(c->type==CATALOG_HOMEBREW && c->count>=4096){c->truncated=true;free(item->homebrew);return true;}
    if(c->count==CATALOG_MAX_ITEMS){c->truncated=true;return true;}
    if(c->count==c->capacity){
        size_t next=c->capacity?c->capacity*2:256;if(next>CATALOG_MAX_ITEMS)next=CATALOG_MAX_ITEMS;
        CatalogItem *items=realloc(c->items,next*sizeof(*items));if(!items)return false;c->items=items;c->capacity=next;
    }
    if(provided)text(item->cover,sizeof(item->cover),provided);
    c->items[c->count++]=*item;return true;
}
const char *catalog_resolve_cover(StoreCatalog*c,size_t index){
    if(!c || index>=c->count)return "";
    CatalogItem*item=&c->items[index];
    if(!item->cover_resolved || item->cover_revision!=c->cover_revision){char provided[256];text(provided,sizeof(provided),item->cover);find_cover(c,item,provided);item->cover_resolved=true;item->cover_revision=c->cover_revision;}
    return item->cover;
}
void catalog_invalidate_covers(StoreCatalog*c){if(c)++c->cover_revision;}
static bool get_size(const char*str,uint64_t*out){
    if(!str || !*str || *str=='-')return false;
    errno=0;char *end;unsigned long long n=strtoull(str,&end,10);
    if(errno || end==str || *end)return false;
    *out=(uint64_t)n;return true;
}
static int load_tsv(StoreCatalog*c,FILE*f){
    char line[8192];size_t bytes=0;
    while(!feof(f)){
        long source_offset=ftell(f);
        if(!fgets(line,sizeof(line),f))break;
        size_t len=strlen(line);bytes+=len;if(bytes>CATALOG_MAX_FILE)return -1;
        if(len==sizeof(line)-1 && line[len-1]!='\n'){
            int ch;while((ch=fgetc(f))!=EOF && ch!='\n'){if(++bytes>CATALOG_MAX_FILE)return -1;}c->skipped++;continue;
        }
        char *columns[24];unsigned count=0;columns[count++]=line;
        for(char*s=line;*s;s++){if(*s=='\t'){*s=0;if(count<24)columns[count++]=s+1;}else if(*s=='\r' || *s=='\n'){*s=0;break;}}
        int title_column=c->type==CATALOG_PSP?3:2;
        int content_column=(c->type==CATALOG_PS1 || c->type==CATALOG_PSP_DLCS)?4:5;
        int size_column=(c->type==CATALOG_GAMES || c->type==CATALOG_DEMOS || c->type==CATALOG_PSP_DLCS)?8:c->type==CATALOG_PSP?9:7;
        if(count<=(unsigned)title_column){c->skipped++;continue;}
        if(!strcmp(columns[0],"Title ID") || !strcmp(columns[0],"TitleID") || !strcmp(columns[0],"title_id"))continue;
        CatalogItem item;memset(&item,0,sizeof(item));
        item.source_offset=source_offset;
        if(launcher_title_id_valid(columns[0]))text(item.title_id,sizeof(item.title_id),columns[0]);
        if(count>(unsigned)content_column){
            text(item.content_id,sizeof(item.content_id),columns[content_column]);
            if(!item.title_id[0] && strlen(item.content_id)>=16){memcpy(item.title_id,item.content_id+7,9);item.title_id[9]=0;}
        }
        text(item.name,sizeof(item.name),columns[title_column]);if(count>1)text(item.region,sizeof(item.region),columns[1]);
        if(count>(unsigned)size_column)item.has_size=get_size(columns[size_column],&item.size_bytes);
        if(!append(c,&item,NULL))return -1;
    }
    return ferror(f)?-1:0;
}
static const cJSON *field(const cJSON*object,const char*a,const char*b,const char*d){
    const cJSON*v=cJSON_GetObjectItemCaseSensitive(object,a);if(!v && b)v=cJSON_GetObjectItemCaseSensitive(object,b);if(!v && d)v=cJSON_GetObjectItemCaseSensitive(object,d);return v;
}
static const char *string_field(const cJSON*object,const char*a,const char*b,const char*d){const cJSON*v=field(object,a,b,d);return cJSON_IsString(v)?v->valuestring:NULL;}
static void homebrew_key(char*out,size_t cap,const char*id,const char*url){
    uint64_t a=UINT64_C(14695981039346656037),b=a;
    for(const unsigned char*s=(const unsigned char*)(id?id:"");*s;s++){a^=*s;a*=UINT64_C(1099511628211);}
    for(const unsigned char*s=(const unsigned char*)(url?url:"");*s;s++){b^=*s;b*=UINT64_C(1099511628211);}
    snprintf(out,cap,"hb_%016llx_%016llx",(unsigned long long)a,(unsigned long long)b);
}
static int load_json(StoreCatalog*c,FILE*f){
    if(fseek(f,0,SEEK_END))return -1;
    long size=ftell(f);
    if(size<2 || size>(long)CATALOG_MAX_FILE || fseek(f,0,SEEK_SET))return -1;
    char *data=malloc((size_t)size+1);if(!data)return -1;
    if(fread(data,1,(size_t)size,f)!=(size_t)size){free(data);return -1;}data[size]=0;
    cJSON *root=cJSON_ParseWithLengthOpts(data,(size_t)size+1,NULL,1);free(data);if(!root)return -1;
    const cJSON *array=root;
    if(cJSON_IsObject(root)){
        array=field(root,json_keys[c->type],"items","data");if(!array)array=field(root,"titles","entries",NULL);
    }
    int result=0;
    if(!cJSON_IsArray(array))result=-1;
    else{
        const cJSON *entry;long ordinal=0;
        cJSON_ArrayForEach(entry,array){
            long current=ordinal++;
            if(!cJSON_IsObject(entry)){c->skipped++;continue;}
            CatalogItem item;memset(&item,0,sizeof(item));
            item.source_offset=current;
            const char*id=string_field(entry,"title_id","titleid","id");
            if(id && strlen(id)==9)text(item.title_id,sizeof(item.title_id),id);
            const char*content=string_field(entry,"content_id","content","contentId");text(item.content_id,sizeof(item.content_id),content);
            if(!item.title_id[0] && strlen(item.content_id)>=16){memcpy(item.title_id,item.content_id+7,9);item.title_id[9]=0;}
            text(item.name,sizeof(item.name),string_field(entry,"name","title","name_org"));
            if(c->type==CATALOG_HOMEBREW){
                item.homebrew=calloc(1,sizeof(*item.homebrew));if(!item.homebrew){result=-1;break;}
                HomebrewMetadata*h=item.homebrew;
                text(h->id,sizeof(h->id),string_field(entry,"id",NULL,NULL));
                text(h->description,sizeof(h->description),string_field(entry,"long_description","description",NULL));
                text(h->requirements,sizeof(h->requirements),string_field(entry,"requirements",NULL,NULL));
                text(h->version,sizeof(h->version),string_field(entry,"version",NULL,NULL));
                text(h->category,sizeof(h->category),string_field(entry,"category_id",NULL,NULL));
                text(h->icon_url,sizeof(h->icon_url),string_field(entry,"icon","cover",NULL));
                const cJSON*authors=field(entry,"author_ids",NULL,NULL);const cJSON*author=cJSON_IsArray(authors)?cJSON_GetArrayItem(authors,0):NULL;
                text(h->author,sizeof(h->author),cJSON_IsString(author)?author->valuestring:"");
                if(!launcher_title_id_valid(item.title_id))item.title_id[0]=0;
                homebrew_key(item.cache_key,sizeof(item.cache_key),h->id,h->icon_url);
            }
            text(item.region,sizeof(item.region),string_field(entry,"region","country",NULL));
            const cJSON*n=field(entry,"size_bytes","size",NULL);
            if(cJSON_IsString(n))item.has_size=get_size(n->valuestring,&item.size_bytes);
            else if(cJSON_IsNumber(n) && n->valuedouble>=0 && n->valuedouble<=9007199254740991.0 && floor(n->valuedouble)==n->valuedouble){item.size_bytes=(uint64_t)n->valuedouble;item.has_size=true;}
            if(!append(c,&item,string_field(entry,"cover_path","cover","image"))){free(item.homebrew);result=-1;break;}
        }
    }
    cJSON_Delete(root);return result;
}
static bool load_path(StoreCatalog*c,const char*root,CatalogType type,const char*path,bool json,const char*filter){
    StoreCatalog next;catalog_init(&next);next.type=type;next.source_json=json;text(next.root,sizeof(next.root),root);text(next.source_path,sizeof(next.source_path),path);configure_covers(&next);
    if(filter)text(next.title_filter,sizeof(next.title_filter),filter);
    FILE*f=fopen(path,"rb");if(!f){c->status=CATALOG_MISSING;return false;}
    int result=json?load_json(&next,f):load_tsv(&next,f);if(fclose(f))result=-1;
    if(result<0){catalog_clear(&next);c->status=CATALOG_INVALID;vs_log("catalog","invalid or unreadable catalog",-1);return false;}
    next.status=CATALOG_READY;catalog_clear(c);*c=next;return true;
}
static bool find_source(char*path,size_t cap,const char*root,CatalogType type,bool json){
    char name[64];snprintf(name,sizeof(name),"%s.%s",files[type],json?"json":"tsv");
    if(path_join(path,cap,root,name) && exists(path))return true;
    if(json){snprintf(name,sizeof(name),"%s.json",json_keys[type]);if(path_join(path,cap,root,name) && exists(path))return true;if(path_join(path,cap,root,"catalog.json") && exists(path))return true;}
    return false;
}
bool catalog_load_matching_root(StoreCatalog*c,const char*root,CatalogType type,const char*filter){
    if(!c || !root || !*root || strlen(root)>=256 || (unsigned)type>=CATALOG_TYPE_COUNT)return false;
    char path[256];
    if(filter && !launcher_title_id_valid(filter))return false;
    for(int json=0;json<2;json++)if(find_source(path,sizeof(path),root,type,json!=0))return load_path(c,root,type,path,json!=0,filter);
    catalog_clear(c);c->type=type;c->status=CATALOG_MISSING;text(c->root,sizeof(c->root),root);return false;
}
bool catalog_load_root(StoreCatalog*c,const char*root,CatalogType type){return catalog_load_matching_root(c,root,type,NULL);}
bool catalog_load_matching_auto(StoreCatalog*c,CatalogType type,const char*filter){
    if(!c || (unsigned)type>=CATALOG_TYPE_COUNT)return false;
    static const char*roots[]={"ux0:usagi-pkgj","ur0:usagi-pkgj","ux0:pkgj","ur0:pkgj"};
    char path[256];
    if(type==CATALOG_HOMEBREW){
        const char*homebrew_roots[]={"ux0:data/VitaStation/alive","app0:assets/alive"};
        for(unsigned r=0;r<2;r++)if(find_source(path,sizeof(path),homebrew_roots[r],type,true))return load_path(c,homebrew_roots[r],type,path,true,filter);
        catalog_clear(c);c->type=type;c->status=CATALOG_MISSING;return false;
    }
    if(filter && !launcher_title_id_valid(filter))return false;
    for(int json=0;json<2;json++)for(unsigned r=0;r<4;r++)if(find_source(path,sizeof(path),roots[r],type,json!=0))return load_path(c,roots[r],type,path,json!=0,filter);
    catalog_clear(c);c->type=type;c->status=CATALOG_MISSING;return false;
}
bool catalog_load_auto(StoreCatalog*c,CatalogType type){return catalog_load_matching_auto(c,type,NULL);}

static bool exact_field(char *out,size_t cap,const char *s){
    if(!s)s="";
    if(strlen(s)>=cap)return false;
    memcpy(out,s,strlen(s)+1);return true;
}
bool catalog_get_package(const StoreCatalog*c,size_t index,CatalogPackage*p,char*error,size_t cap){
    if(c && c->type==CATALOG_HOMEBREW && c->source_json){
        CatalogLink links[24];size_t count=catalog_homebrew_links(c,index,links,24),chosen=count;
        for(size_t i=0;i<count;i++)if(links[i].format==CATALOG_LINK_VPK || links[i].format==CATALOG_LINK_APP_ZIP){if(chosen==count || links[i].recommended)chosen=i;if(links[i].recommended)break;}
        if(chosen<count)return catalog_homebrew_package(c,index,&links[chosen],p,error,cap);
        if(error && cap)snprintf(error,cap,"Este item nao tem uma ligacao de instalacao disponivel.");
        return false;
    }
    const CatalogItem*item=catalog_get(c,index);
    const char *failure="O catalogo foi alterado. Atualiza a categoria.";
    if(!p || !item || c->status!=CATALOG_READY)goto fail;
    memset(p,0,sizeof(*p));p->type=c->type;
    text(p->title_id,sizeof(p->title_id),item->title_id);text(p->name,sizeof(p->name),item->name);
    text(p->content_id,sizeof(p->content_id),item->content_id);p->size_bytes=item->size_bytes;
    FILE*f=fopen(c->source_path,"rb");if(!f)goto fail;
    bool ok=false;
    if(!c->source_json){
        char line[8192];char*col[24];unsigned count=1;
        if(fseek(f,item->source_offset,SEEK_SET)==0 && fgets(line,sizeof(line),f)){
            col[0]=line;
            for(char*s=line;*s;s++){if(*s=='\t'){*s=0;if(count<24)col[count++]=s+1;}else if(*s=='\r'||*s=='\n'){*s=0;break;}}
            unsigned ci=(c->type==CATALOG_PS1 || c->type==CATALOG_PSP_DLCS)?4:5;
            unsigned ui=c->type==CATALOG_PSP?4:3;
            unsigned di=(c->type==CATALOG_GAMES || c->type==CATALOG_DEMOS || c->type==CATALOG_PSP_DLCS)?9:c->type==CATALOG_PSP?10:8;
            bool licensed=(c->type<=CATALOG_DEMOS || c->type==CATALOG_PSM);
            ok=count>ci && count>ui && !strcmp(col[ci],item->content_id) &&
                exact_field(p->url,sizeof(p->url),col[ui]) &&
                exact_field(p->zrif,sizeof(p->zrif),licensed && count>4?col[4]:"") &&
                exact_field(p->digest,sizeof(p->digest),count>di?col[di]:"");
        }
    }else if(!fseek(f,0,SEEK_END)){
        long len=ftell(f);
        if(len>1 && len<=(long)CATALOG_MAX_FILE && !fseek(f,0,SEEK_SET)){
            char*data=malloc((size_t)len+1);
            if(data){
                if(fread(data,1,(size_t)len,f)==(size_t)len){
                    data[len]=0;cJSON*root=cJSON_ParseWithLengthOpts(data,(size_t)len+1,NULL,1);
                    const cJSON*array=root;
                    if(cJSON_IsObject(root)){array=field(root,json_keys[c->type],"items","data");if(!array)array=field(root,"titles","entries",NULL);}
                    const cJSON*entry=cJSON_IsArray(array)?cJSON_GetArrayItem(array,(int)item->source_offset):NULL;
                    const char*content=string_field(entry,"content_id","content","contentId");
                    const char*id=string_field(entry,"title_id","titleid","id");
                    ok=cJSON_IsObject(entry) && ((content && !strcmp(content,item->content_id)) || (id && !item->content_id[0] && !strcmp(id,item->title_id))) &&
                        exact_field(p->url,sizeof(p->url),string_field(entry,"url","download_url","pkg_url")) &&
                        exact_field(p->zrif,sizeof(p->zrif),string_field(entry,"zrif","zRIF","license")) &&
                        exact_field(p->digest,sizeof(p->digest),string_field(entry,"sha256","digest",NULL));
                    cJSON_Delete(root);
                }free(data);
            }
        }
    }
    fclose(f);
    if(!ok)goto fail;
    failure="Este item nao tem um endereco de descarga valido.";
    if(strncmp(p->url,"https://",8) && strncmp(p->url,"http://",7))goto fail;
    for(const char*s=p->url;*s;s++)if((unsigned char)*s<=32)goto fail;
    const char*end=strpbrk(p->url,"?#");size_t length=end?(size_t)(end-p->url):strlen(p->url);
    p->vpk=length>=4 && !strncmp(p->url+length-4,".vpk",4);
    if(!p->vpk){
        failure="Identificador de pacote invalido.";
        size_t len=strlen(p->content_id);if(len!=36 || strncmp(p->content_id+7,p->title_id,9))goto fail;
        for(size_t i=0;i<len;i++)if(!isalnum((unsigned char)p->content_id[i]) && p->content_id[i]!='-' && p->content_id[i]!='_')goto fail;
    }
    if(p->digest[0]){
        failure="A soma SHA-256 deste item e invalida.";
        if(strlen(p->digest)!=64)goto fail;
        for(unsigned i=0;i<64;i++)if(!isxdigit((unsigned char)p->digest[i]))goto fail;
    }
    if(error && cap)error[0]=0;
    return true;
fail:
    if(error && cap)snprintf(error,cap,"%s",failure);
    return false;
}

static cJSON *homebrew_entry(const StoreCatalog*c,size_t index,const cJSON**entry){
    *entry=NULL;const CatalogItem*item=catalog_get(c,index);if(!item || !item->homebrew || !c->source_json)return NULL;
    FILE*f=fopen(c->source_path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<2 || n>(long)CATALOG_MAX_FILE || fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    char*data=malloc((size_t)n+1);if(!data){fclose(f);return NULL;}
    bool ok=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);data[n]=0;
    cJSON*root=ok?cJSON_ParseWithLengthOpts(data,(size_t)n+1,NULL,1):NULL;free(data);if(!root)return NULL;
    const cJSON*array=cJSON_IsArray(root)?root:field(root,"homebrew","items","data");
    const cJSON*object=cJSON_IsArray(array)?cJSON_GetArrayItem(array,(int)item->source_offset):NULL;
    const char*id=string_field(object,"id",NULL,NULL);
    if(!id || strcmp(id,item->homebrew->id)){cJSON_Delete(root);return NULL;}
    *entry=object;return root;
}
static bool http_url(const char*s){
    if(!s || (strncmp(s,"https://",8) && strncmp(s,"http://",7)) || strlen(s)>=1024)return false;
    for(;*s;s++)if((unsigned char)*s<=32)return false;
    return true;
}
static bool url_zip(const char*url){
    const char*end=strpbrk(url,"?#");size_t n=end?(size_t)(end-url):strlen(url);
    return (n>=4 && !strncmp(url+n-4,".zip",4)) || strstr(url,".zip/file")!=NULL;
}
static bool link_filename(char*out,size_t cap,const char*url){
    const char*end=strpbrk(url,"?#");if(!end)end=url+strlen(url);const char*start=end;while(start>url && start[-1]!='/')start--;
    size_t n=(size_t)(end-start);if(!n || n>=cap)return false;
    memcpy(out,start,n);out[n]=0;
    if(!strcmp(out,".") || !strcmp(out,".."))return false;
    for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)out[i]) && out[i]!='.' && out[i]!='-' && out[i]!='_')return false;
    return true;
}
size_t catalog_homebrew_links(const StoreCatalog*c,size_t index,CatalogLink*out,size_t cap){
    if(!out || !cap)return 0;
    const cJSON*entry;cJSON*root=homebrew_entry(c,index,&entry);if(!root)return 0;
    const cJSON*links=field(entry,"links",NULL,NULL),*link;size_t count=0;
    cJSON_ArrayForEach(link,links){
        if(count==cap)break;
        const char*url=string_field(link,"url",NULL,NULL),*type=string_field(link,"type",NULL,NULL);
        if(!http_url(url) || !type)continue;
        CatalogLink item;memset(&item,0,sizeof(item));item.format=CATALOG_LINK_REFERENCE;
        text(item.name,sizeof(item.name),string_field(link,"name","type",NULL));exact_field(item.url,sizeof(item.url),url);
        if(!strcmp(type,"Download") || (!strcmp(type,"Mirror") && (strstr(url,".vpk") || url_zip(url))))item.format=url_zip(url)?CATALOG_LINK_APP_ZIP:CATALOG_LINK_VPK;
        else if(!strcmp(type,"Data Files") || !strcmp(type,"Game Files") || !strcmp(type,"Mod") || !strcmp(type,"Mod Pack") || !strcmp(type,"Patch"))item.format=url_zip(url)?CATALOG_LINK_DATA_ZIP:CATALOG_LINK_DATA_FILE;
        else if(!strcmp(type,"Plugin"))item.format=CATALOG_LINK_PLUGIN;
        const cJSON*size=field(link,"size",NULL,NULL);
        if(cJSON_IsNumber(size) && size->valuedouble>=0 && size->valuedouble<=34359738368.0)item.size_bytes=(uint64_t)size->valuedouble;
        else if(cJSON_IsString(size))get_size(size->valuestring,&item.size_bytes);
        item.recommended=cJSON_IsTrue(field(link,"recommended",NULL,NULL));item.required=cJSON_IsTrue(field(link,"required",NULL,NULL));
        const char*path=string_field(link,"extract_path",NULL,NULL),*section=string_field(link,"section",NULL,NULL);
        if(path && !exact_field(item.extract_path,sizeof(item.extract_path),path))item.format=CATALOG_LINK_REFERENCE;
        if(section && !exact_field(item.section,sizeof(item.section),section))item.format=CATALOG_LINK_REFERENCE;
        link_filename(item.filename,sizeof(item.filename),url);
        out[count++]=item;
    }
    cJSON_Delete(root);return count;
}
bool catalog_homebrew_package(const StoreCatalog*c,size_t index,const CatalogLink*link,CatalogPackage*p,char*error,size_t cap){
    const CatalogItem*item=catalog_get(c,index);const char*message="Esta ligacao e informativa; nao e um instalador.";
    if(!item || !item->homebrew || !link || !p || !http_url(link->url) || link->format==CATALOG_LINK_REFERENCE)goto fail;
    message="O destino dos ficheiros nao esta indicado no catalogo.";
    if((link->format==CATALOG_LINK_DATA_ZIP || link->format==CATALOG_LINK_DATA_FILE) && (!link->extract_path[0] || strstr(link->extract_path,"..") || (strncmp(link->extract_path,"ux0:",4) && strncmp(link->extract_path,"ur0:",4))))goto fail;
    message="Nao foi possivel determinar o nome do ficheiro.";
    if((link->format==CATALOG_LINK_DATA_FILE || link->format==CATALOG_LINK_PLUGIN) && !link->filename[0])goto fail;
    memset(p,0,sizeof(*p));p->type=CATALOG_HOMEBREW;p->format=link->format;p->size_bytes=link->size_bytes;
    p->vpk=link->format==CATALOG_LINK_VPK || link->format==CATALOG_LINK_APP_ZIP;
    text(p->title_id,sizeof(p->title_id),item->title_id);text(p->name,sizeof(p->name),item->name);text(p->cache_key,sizeof(p->cache_key),item->cache_key);
    text(p->url,sizeof(p->url),link->url);text(p->extract_path,sizeof(p->extract_path),link->extract_path);text(p->plugin_section,sizeof(p->plugin_section),link->section);text(p->filename,sizeof(p->filename),link->filename);
    if(error && cap)error[0]=0;
    return true;
fail:
    if(error && cap)snprintf(error,cap,"%s",message);
    return false;
}
