#include "autoplugin_parser.hpp"
static void integration_message(const std::string&s){lock();snprintf(integration_status.message,sizeof(integration_status.message),"%s",s.c_str());unlock();}
static std::string json_text(const cJSON*j,const char*key){auto*v=cJSON_GetObjectItemCaseSensitive(j,key);return cJSON_IsString(v)?v->valuestring:"";}
static void refresh_autoplugin(){
 auto bytes=https_bytes("https://api.github.com/repos/ONElua/AutoPlugin2/commits/master",1024*1024);std::string json(bytes.begin(),bytes.end());cJSON*j=cJSON_Parse(json.c_str());if(!j)throw std::runtime_error("Resposta AutoPlugin II invalida.");auto commit=json_text(j,"sha");cJSON_Delete(j);if(commit.size()!=40 || commit.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("Versao AutoPlugin II invalida.");
 bytes=https_bytes("https://raw.githubusercontent.com/ONElua/AutoPlugin2/"+commit+"/plugins/plugins.lua",2*1024*1024);auto items=ap_parse(std::string(bytes.begin(),bytes.end()));if(items.empty())throw std::runtime_error("Catalogo AutoPlugin II vazio ou alterado.");
 pkgi_mkdirs("ux0:data/VitaStation/autoplugin");pkgi_save("ux0:data/VitaStation/autoplugin/plugins.lua",bytes.data(),bytes.size());
 lock();online_plugins=std::move(items);autoplugin_commit=commit;integration_status.count=online_plugins.size();unlock();integration_message("Catalogo AutoPlugin II atualizado. X: obter plugin; Triangulo: atualizar lista.");
}
static void install_autoplugin(int index){
 lock();if(index<0 || (size_t)index>=online_plugins.size()){unlock();throw std::runtime_error("Seleciona um plugin.");}OnlinePlugin p=online_plugins[index];std::string commit=autoplugin_commit;unlock();
 auto path=std::string("AUTOPLUG2/resources/plugins/")+p.file;
 auto info=https_bytes("https://api.github.com/repos/ONElua/AutoPlugin2/contents/"+path+"?ref="+commit,1024*1024);std::string json(info.begin(),info.end());cJSON*j=cJSON_Parse(json.c_str());if(!j)throw std::runtime_error("Metadados do plugin invalidos.");auto expected=json_text(j,"sha");cJSON_Delete(j);if(expected.size()!=40)throw std::runtime_error("O catalogo nao forneceu a verificacao do plugin.");
 auto bytes=https_bytes("https://raw.githubusercontent.com/ONElua/AutoPlugin2/"+commit+"/"+path,8*1024*1024);auto head=std::string("blob ")+std::to_string(bytes.size())+'\0';std::vector<uint8_t>blob(head.begin(),head.end());blob.insert(blob.end(),bytes.begin(),bytes.end());uint8_t sum[20];SHA1(blob.data(),blob.size(),sum);char hex[41];for(unsigned i=0;i<20;i++)snprintf(hex+2*i,3,"%02x",sum[i]);if(expected!=hex || (p.crc && (unsigned)crc32(0,bytes.data(),bytes.size())!=p.crc))throw std::runtime_error("O plugin falhou a verificacao do catalogo.");
 const std::string downloads="ux0:data/VitaStation/autoplugin/downloads";pkgi_mkdirs(downloads.c_str());pkgi_save(downloads+"/"+p.file,bytes.data(),bytes.size());
 if(!p.automatic){integration_message("Plugin descarregado e verificado. Instalacao especial ainda sem adaptador; nao foi ativado.");return;}
 if(bytes.size()<4 || memcmp(bytes.data(),"SCE\0",4))throw std::runtime_error("Modulo de plugin invalido.");
 char error[256];if(!safety_backup_ensure(error,sizeof(error)))throw std::runtime_error(error);
 const std::string config=pkgi_file_exists("ux0:tai/config.txt")?"ux0:tai/config.txt":"ur0:tai/config.txt";auto data=pkgi_load(config);std::string text(data.begin(),data.end());std::vector<PluginEntry>entries(256);auto count=plugins_config_entries(text.c_str(),entries.data(),entries.size());
 std::string root=config.substr(0,config.find_last_of('/'))+"/VitaStation",target=root+"/"+p.file;
 for(size_t i=0;i<count;i++){auto name=std::string(entries[i].path);auto slash=name.find_last_of('/');if(slash!=std::string::npos)name=name.substr(slash+1);auto expected=std::string(p.file);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return (char)std::tolower(c);});std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return (char)std::tolower(c);});if(name==expected){if(!entries[i].enabled)throw std::runtime_error("Plugin existente desativado; usa Gerir plugins para o ativar.");target=entries[i].path;break;}}
 std::vector<char>output(128*1024);int changed=plugins_config_add(text.c_str(),p.section,target.c_str(),output.data(),output.size());if(changed<0)throw std::runtime_error("Conflito com a configuracao existente; preservada.");
 save_support(target,bytes);
 if(changed){std::string next(output.data());save_support(config,std::vector<uint8_t>(next.begin(),next.end()));}
 integration_message("Plugin instalado/atualizado. Reinicia a consola para aplicar.");
}
static void apollo_data_for_game(const AppItem&app){
 if(strncmp(app.title_id,"PCS",3))return;
 auto bytes=https_bytes(std::string("https://raw.githubusercontent.com/bucanero/apollo-patches/main/PSV/")+app.title_id+".savepatch",1024*1024);
 std::string file=std::string(app.title_id)+".savepatch";save_support("ux0:data/VitaStation/apollo/"+file,bytes,true);
 if(pkgi_file_exists("ux0:app/NP0APOLLO/sce_sys/param.sfo"))save_support("ux0:app/NP0APOLLO/data/"+file,bytes,true);
}
static void prepare_apollo(){
 if(!pkgi_file_exists("ux0:app/NP0APOLLO/sce_sys/param.sfo")){
  integration_message("A procurar o Apollo e preparar a instalacao...");auto bytes=https_bytes("https://api.github.com/repos/bucanero/apollo-vita/releases/latest",2*1024*1024);std::string text(bytes.begin(),bytes.end());cJSON*j=cJSON_Parse(text.c_str());CatalogPackage p{};p.type=CATALOG_GAMES;p.vpk=true;snprintf(p.title_id,sizeof(p.title_id),"NP0APOLLO");snprintf(p.name,sizeof(p.name),"Apollo Save Tool");bool found=false;
  if(j){const cJSON*a;cJSON_ArrayForEach(a,cJSON_GetObjectItemCaseSensitive(j,"assets")){auto name=json_text(a,"name"),url=json_text(a,"browser_download_url"),digest=json_text(a,"digest");if(name.size()<4 || name.substr(name.size()-4)!=".vpk" || url.rfind("https://github.com/bucanero/apollo-vita/releases/download/",0) || digest.size()!=71 || digest.rfind("sha256:",0))continue;snprintf(p.url,sizeof(p.url),"%s",url.c_str());snprintf(p.digest,sizeof(p.digest),"%s",digest.c_str()+7);found=true;break;}cJSON_Delete(j);}if(!found)throw std::runtime_error("Apollo sem VPK verificavel nesta Release.");run_job(p);
 }
 if(pkgi_get_inode_type("ux0:data/VitaStation/apollo")==InodeType::Directory)for(auto&file:pkgi_list_dir_contents("ux0:data/VitaStation/apollo")){if(!safe_relative(file) || file.find('/')!=std::string::npos || file.find(".savepatch")==std::string::npos)continue;auto bytes=pkgi_load("ux0:data/VitaStation/apollo/"+file);save_support("ux0:app/NP0APOLLO/data/"+file,bytes,true);}
 lock();apollo_ready=true;unlock();integration_message("Apollo preparado. A abrir a gestao de saves...");
}
extern "C" void store_integration_status(IntegrationStatus*out){if(!out)return;if(lock_id<0){*out={};return;}lock();*out=integration_status;unlock();}
extern "C" bool store_autoplugin_item(int index,OnlinePlugin*out){if(!out || lock_id<0)return false;lock();bool ok=index>=0 && (size_t)index<online_plugins.size();if(ok)*out=online_plugins[index];unlock();return ok;}
extern "C" void store_autoplugin_request(int index){if(lock_id<0 || cover_thread_id<0)return;lock();if(integration_status.busy || update_busy || game_status.busy || progress.active || count){unlock();return;}integration_status.busy=true;integration_pending=true;integration_action=index;snprintf(integration_status.message,sizeof(integration_status.message),"A consultar o AutoPlugin II...");unlock();sceKernelSignalSema(cover_wake_id,1);}
extern "C" void store_apollo_request(){if(lock_id<0 || cover_thread_id<0)return;lock();if(integration_status.busy || update_busy || game_status.busy || progress.active || count){unlock();return;}integration_status.busy=true;integration_pending=true;integration_action=-2;unlock();sceKernelSignalSema(cover_wake_id,1);}
extern "C" bool store_apollo_ready(){if(lock_id<0)return false;lock();bool ready=apollo_ready;apollo_ready=false;unlock();return ready;}
