/* Verified, immutable plugin baseline. Restore commits configuration last. */
#include "safety_backup.h"
#include "../third_party/cJSON/cJSON.h"
#include "../third_party/usagi/sha256.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <stdexcept>
#if defined(__vita__)
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#else
#include <filesystem>
#endif
namespace {
const std::string base="ux0:data/VitaStation/safety-backup";
const std::vector<std::string> defaults={"ur0:tai","ux0:tai","ux0:pspemu/seplugins","ux0:app/PSPEMUCFW/sce_module","ux0:vitacheat"};
std::mutex gate;
struct Entry{std::string path,hash,blob;int type;uint64_t size;};
struct Snapshot{std::vector<std::string>roots;std::vector<Entry>entries;bool legacy=false;};
std::string disk(std::string p){
#if !defined(__vita__)
 auto at=p.find(':');if(at!=std::string::npos)p.replace(at,1,"/");return "testfs/"+p;
#else
 return p;
#endif
}
void fail(const char*s){throw std::runtime_error(s);}
int kind(const std::string&p,uint64_t*size=nullptr){
#if defined(__vita__)
 SceIoStat st{};int n=sceIoGetstat(p.c_str(),&st);if(n==(int)0x80010002)return 0;if(n<0)fail("Sem acesso a um caminho de plugins.");if(size)*size=st.st_size;return SCE_S_ISDIR(st.st_mode)?2:1;
#else
 std::error_code e;auto st=std::filesystem::symlink_status(disk(p),e);if(e==std::errc::no_such_file_or_directory || st.type()==std::filesystem::file_type::not_found)return 0;if(e || std::filesystem::is_symlink(st))fail("Invalid filesystem entry");if(std::filesystem::is_directory(st))return 2;if(!std::filesystem::is_regular_file(st))fail("Invalid file");if(size)*size=std::filesystem::file_size(disk(p));return 1;
#endif
}
void mkdirs(const std::string&p){
#if defined(__vita__)
 for(size_t at=p.find('/');;at=p.find('/',at+1)){auto part=p.substr(0,at);if(!part.empty() && !kind(part) && sceIoMkdir(part.c_str(),0777)<0)fail("Nao foi possivel criar a pasta de seguranca.");if(at==std::string::npos)break;}
#else
 std::filesystem::create_directories(disk(p));
#endif
}
void parent(const std::string&p){auto at=p.find_last_of('/');if(at!=std::string::npos)mkdirs(p.substr(0,at));}
void move(const std::string&a,const std::string&b){
#if defined(__vita__)
 if(sceIoRename(a.c_str(),b.c_str())<0)fail("Falha ao trocar ficheiros na reposicao.");
#else
 std::filesystem::rename(disk(a),disk(b));
#endif
}
void remove_file(const std::string&p){if(!kind(p))return;
#if defined(__vita__)
 if(sceIoRemove(p.c_str())<0)fail("Falha ao remover ficheiro da reposicao.");
#else
 std::filesystem::remove(disk(p));
#endif
}
void remove_empty(const std::string&p){
#if defined(__vita__)
 sceIoRmdir(p.c_str());
#else
 std::error_code e;std::filesystem::remove(disk(p),e);
#endif
}
std::vector<std::string>children(const std::string&p){std::vector<std::string>out;
#if defined(__vita__)
 SceUID d=sceIoDopen(p.c_str());if(d<0)fail("Nao foi possivel ler a pasta de plugins.");int n;while(1){SceIoDirent e{};n=sceIoDread(d,&e);if(n<=0)break;std::string name=e.d_name;if(name!="." && name!="..")out.push_back(name);}sceIoDclose(d);if(n<0)fail("Leitura incompleta de plugins.");
#else
 for(auto&e:std::filesystem::directory_iterator(disk(p)))out.push_back(e.path().filename().string());
#endif
 std::sort(out.begin(),out.end());return out;}
bool safe(const std::string&p){return p.size()<800 && p.find("..") == std::string::npos && p.find_first_of("\r\n\t\\")==std::string::npos && p.find(':')!=std::string::npos && (p.rfind("ux0:",0)==0 || p.rfind("ur0:",0)==0 || p.rfind("uma0:",0)==0 || p.rfind("imc0:",0)==0 || p.rfind("xmc0:",0)==0);}
bool suffix(const std::string&p,const char*s){size_t n=strlen(s);return p.size()>=n && p.compare(p.size()-n,n,s)==0;}
bool root_allowed(const std::string&p){return safe(p) && (std::find(defaults.begin(),defaults.end(),p)!=defaults.end() || suffix(p,".suprx") || suffix(p,".skprx") || suffix(p,".prx"));}
std::string hash_file(const std::string&p,const std::string*dest=nullptr){FILE*in=fopen(disk(p).c_str(),"rb");if(!in)fail("Nao foi possivel ler o backup de plugins.");FILE*out=nullptr;if(dest){parent(*dest);out=fopen(disk(*dest).c_str(),"wb");if(!out){fclose(in);fail("Nao foi possivel gravar o backup.");}}
 sha256_ctx ctx;sha256_init(&ctx);unsigned char buffer[65536];size_t n;bool ok=true;uint64_t total=0;
 while((n=fread(buffer,1,sizeof(buffer),in))){total+=n;if(total>64*1024*1024 || (out && fwrite(buffer,1,n,out)!=n)){ok=false;break;}sha256_update(&ctx,buffer,n);}
 if(ferror(in))ok=false;
 if(fclose(in))ok=false;
 if(out && fclose(out))ok=false;
 if(!ok)fail("Copia de plugins incompleta; alteracoes bloqueadas.");
 uint8_t digest[32];sha256_finish(&ctx,digest);char hex[65];for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);return hex;}
void walk(const std::string&p,std::vector<Entry>&list,unsigned depth=0){if(depth>24 || list.size()>=16384 || !safe(p))fail("Limites da copia de seguranca excedidos.");uint64_t size=0;int t=kind(p,&size);list.push_back({p,"","",t,size});if(t==2)for(auto&name:children(p)){if(name=="." || name==".." || name.find_first_of("/\\\r\n\t")!=std::string::npos)fail("Nome de plugin invalido.");walk(p+"/"+name,list,depth+1);}}
const char*str(const cJSON*j,const char*k){auto*v=cJSON_GetObjectItemCaseSensitive(j,k);return cJSON_IsString(v)?v->valuestring:"";}
std::string text_file(const std::string&p,size_t limit){uint64_t size=0;if(kind(p,&size)!=1 || size>limit)fail("Manifesto/configuracao de plugins invalido.");FILE*f=fopen(disk(p).c_str(),"rb");if(!f)fail("Nao foi possivel ler a configuracao.");std::string s(size,'\0');bool ok=fread(s.data(),1,s.size(),f)==s.size();if(fclose(f))ok=false;if(!ok || s.find('\0')!=std::string::npos)fail("Configuracao incompleta.");return s;}
void discover(Snapshot&s){s.roots=defaults;for(const char*cfg:{"ur0:tai/config.txt","ux0:tai/config.txt"}){if(!kind(cfg))continue;auto text=text_file(cfg,128*1024);if(text.find("VitaStation")!=std::string::npos)s.legacy=true;for(size_t at=0;at<text.size();){auto end=text.find('\n',at);if(end==std::string::npos)end=text.size();auto line=text.substr(at,end-at);at=end+1;auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos || line[first]=='#' || line[first]=='*')continue;line=line.substr(first);line=line.substr(0,line.find_first_of(" \t\r#"));if(!root_allowed(line))continue;bool covered=false;for(auto&r:defaults)if(line==r || line.rfind(r+"/",0)==0)covered=true;if(!covered && std::find(s.roots.begin(),s.roots.end(),line)==s.roots.end())s.roots.push_back(line);}}}
Snapshot load(const std::string&folder){auto bytes=text_file(folder+"/manifest.json",8*1024*1024);cJSON*j=cJSON_Parse(bytes.c_str());if(!j)fail("Manifesto de seguranca danificado.");Snapshot s;try{auto*roots=cJSON_GetObjectItemCaseSensitive(j,"roots"),*entries=cJSON_GetObjectItemCaseSensitive(j,"entries");if(!cJSON_IsArray(roots) || !cJSON_IsArray(entries))fail("Backup incompleto.");const cJSON*r; cJSON_ArrayForEach(r,roots){if(!cJSON_IsString(r) || !root_allowed(r->valuestring))fail("Raiz de backup invalida.");s.roots.push_back(r->valuestring);}for(auto&r:defaults)if(std::find(s.roots.begin(),s.roots.end(),r)==s.roots.end())fail("Backup sem raiz obrigatoria.");s.legacy=cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(j,"legacy"));const cJSON*e;cJSON_ArrayForEach(e,entries){auto*t=cJSON_GetObjectItemCaseSensitive(e,"type"),*size=cJSON_GetObjectItemCaseSensitive(e,"size");if(!cJSON_IsNumber(t) || !cJSON_IsNumber(size) || size->valuedouble<0 || size->valuedouble>64*1024*1024)fail("Entrada de backup invalida.");Entry item{str(e,"path"),str(e,"hash"),str(e,"blob"),t->valueint,(uint64_t)size->valuedouble};bool allowed=false;for(auto&r:s.roots)if(item.path==r || item.path.rfind(r+"/",0)==0)allowed=true;if(!safe(item.path) || !allowed || item.type<0 || item.type>2 || s.entries.size()>=16384)fail("Caminho de reposicao invalido.");for(auto&old:s.entries)if(old.path==item.path)fail("Entrada duplicada no backup.");if(item.type==1){if(item.blob.empty() || item.blob.find_first_not_of("0123456789")!=std::string::npos || item.hash.size()!=64)fail("Bloco de backup invalido.");uint64_t actual=0;if(kind(folder+"/"+item.blob,&actual)!=1 || actual!=item.size || hash_file(folder+"/"+item.blob)!=item.hash)fail("Backup de plugins falhou SHA-256. Reposicao bloqueada.");}s.entries.push_back(item);}for(auto&r:s.roots){bool found=false;for(auto&e:s.entries)if(e.path==r)found=true;if(!found)fail("Backup incompleto: raiz em falta.");}cJSON_Delete(j);return s;}catch(...){cJSON_Delete(j);throw;}}
void capture(const std::string&folder){if(kind(folder))fail("Pasta de backup existente preservada.");Snapshot s;discover(s);mkdirs(folder);uint64_t total=0;for(auto&r:s.roots)walk(r,s.entries);for(size_t i=0;i<s.entries.size();i++){auto&e=s.entries[i];if(e.type==1){total+=e.size;if(total>512*1024*1024)fail("Backup maior que 512 MiB; alteracoes bloqueadas.");e.blob=std::to_string(i);auto destination=folder+"/"+e.blob;e.hash=hash_file(e.path,&destination);if(hash_file(folder+"/"+e.blob)!=e.hash)fail("Verificacao da copia falhou.");}}
 cJSON*j=cJSON_CreateObject(),*roots=cJSON_AddArrayToObject(j,"roots"),*items=cJSON_AddArrayToObject(j,"entries");cJSON_AddBoolToObject(j,"legacy",s.legacy);for(auto&r:s.roots)cJSON_AddItemToArray(roots,cJSON_CreateString(r.c_str()));for(auto&e:s.entries){auto*v=cJSON_CreateObject();cJSON_AddStringToObject(v,"path",e.path.c_str());cJSON_AddNumberToObject(v,"type",e.type);cJSON_AddNumberToObject(v,"size",e.size);cJSON_AddStringToObject(v,"hash",e.hash.c_str());cJSON_AddStringToObject(v,"blob",e.blob.c_str());cJSON_AddItemToArray(items,v);}char*data=cJSON_PrintUnformatted(j);if(!data){cJSON_Delete(j);fail("Sem memoria para manifesto.");}FILE*f=fopen(disk(folder+"/manifest.json").c_str(),"wb");bool ok=f && fwrite(data,1,strlen(data),f)==strlen(data);if(f && fclose(f))ok=false;cJSON_free(data);cJSON_Delete(j);if(!ok)fail("Nao foi possivel concluir o manifesto.");load(folder);
#if defined(__vita__)
 if(sceIoSync("ux0:",0)<0)fail("Falha ao sincronizar a copia de seguranca.");
#endif
}
bool is_config(const std::string&p){return suffix(p,"/config.txt") || suffix(p,"/game.txt") || suffix(p,"/vsh.txt");}
void restore(const std::string&folder,const Snapshot&s){std::vector<Entry>current;for(auto&r:s.roots)walk(r,current);struct Change{std::string path;bool old=false,applied=false;};std::vector<Change>changes;std::vector<std::string>stages;
 try{for(auto&e:s.entries)if(e.type==2)mkdirs(e.path);for(auto&e:s.entries)if(e.type==1){auto stage=e.path+".vst-restore.new";if(kind(stage) || kind(e.path+".vst-restore.previous"))fail("Recuperacao anterior pendente; nao sobrescrever.");parent(stage);if(hash_file(folder+"/"+e.blob,&stage)!=e.hash)fail("Copia de reposicao falhou.");stages.push_back(stage);if(hash_file(stage)!=e.hash)fail("Verificacao de reposicao falhou.");}
 std::vector<std::string>paths;for(auto&e:current)if(e.type==1)paths.push_back(e.path);for(auto&e:s.entries)if(e.type==1 && std::find(paths.begin(),paths.end(),e.path)==paths.end())paths.push_back(e.path);std::stable_sort(paths.begin(),paths.end(),[](const std::string&a,const std::string&b){return is_config(a)<is_config(b);});
 for(auto&p:paths){if(kind(p+".vst-restore.previous"))fail("Recuperacao pendente ao repor plugins.");changes.push_back({p});auto&c=changes.back();if(kind(p)){if(kind(p)!=1)fail("Conflito de tipos na reposicao.");move(p,p+".vst-restore.previous");c.old=true;}if(kind(p+".vst-restore.new")){move(p+".vst-restore.new",p);c.applied=true;}}
#if defined(__vita__)
 if(sceIoSync("ux0:",0)<0 || sceIoSync("ur0:",0)<0)fail("Falha ao sincronizar a reposicao.");
#endif
 }catch(...){for(auto it=changes.rbegin();it!=changes.rend();++it){try{if(it->applied)remove_file(it->path);if(it->old)move(it->path+".vst-restore.previous",it->path);}catch(...){}}for(auto&p:stages){try{remove_file(p);}catch(...){}}throw;}
 for(auto&c:changes)if(c.old)remove_file(c.path+".vst-restore.previous");
 for(auto it=current.rbegin();it!=current.rend();++it)if(it->type==2){bool keep=false;for(auto&e:s.entries)if(e.path==it->path && e.type==2)keep=true;if(!keep)remove_empty(it->path);}
}
}
extern "C" bool safety_backup_ensure(char*message,size_t cap){std::lock_guard<std::mutex>guard(gate);try{const auto folder=base+"/baseline";Snapshot s;if(kind(folder+"/manifest.json"))s=load(folder);else{if(kind(folder))fail("Backup incompleto preservado; repara a copia antes de alterar plugins.");capture(folder);s=load(folder);}if(message && cap)snprintf(message,cap,s.legacy?"Backup atual guardado; instalacao anterior detetada.":"Backup original de plugins verificado.");return true;}catch(const std::exception&e){if(message && cap)snprintf(message,cap,"%s",e.what());return false;}}
extern "C" bool safety_backup_restore(char*message,size_t cap){std::lock_guard<std::mutex>guard(gate);try{auto folder=base+"/baseline";if(!kind(folder+"/manifest.json"))fail("Sem backup original. Remocao automatica bloqueada; usa recuperacao manual.");auto s=load(folder);if(!kind(base+"/before-uninstall/manifest.json"))capture(base+"/before-uninstall");Snapshot configs;configs.legacy=s.legacy;configs.roots={"ur0:tai/config.txt","ux0:tai/config.txt","ux0:pspemu/seplugins/game.txt","ux0:pspemu/seplugins/vsh.txt","ux0:pspemu/seplugins/pops.txt"};for(auto&path:configs.roots){bool found=false;for(auto&e:s.entries)if(e.path==path){configs.entries.push_back(e);found=true;break;}if(!found)configs.entries.push_back({path,"","",0,0});}for(auto&e:s.entries)if(e.type==1 && (suffix(e.path,".skprx") || suffix(e.path,".suprx") || suffix(e.path,".prx")) && !kind(e.path)){configs.roots.push_back(e.path);configs.entries.push_back(e);}restore(folder,configs);snprintf(message,cap,"Configuracoes repostas; todos os ficheiros de plugins mantidos.");return true;}catch(const std::exception&e){snprintf(message,cap,"%s",e.what());return false;}}

extern "C" bool safety_backup_exists(void){try{return kind(base+"/baseline/manifest.json")==1;}catch(...){return false;}}

namespace {
std::string filename(const std::string&p){auto at=p.find_last_of('/');return p.substr(at==std::string::npos?0:at+1);}
bool preserved_plugin(const Snapshot&s,const std::string&path){if(path.rfind("ux0:app/PSPEMUCFW/",0)==0)return true;auto name=filename(path);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return (char)std::tolower(c);});for(auto&e:s.entries)if(e.type==1){auto original=filename(e.path);std::transform(original.begin(),original.end(),original.begin(),[](unsigned char c){return (char)std::tolower(c);});if(original==name)return true;}return false;}
std::vector<Entry>owned_plugins(){const auto file=base+"/owned-plugins.json";if(!kind(file))return {};auto data=text_file(file,1024*1024);cJSON*j=cJSON_Parse(data.c_str());if(!j || !cJSON_IsArray(j)){cJSON_Delete(j);fail("Registo de plugins adicionados invalido.");}std::vector<Entry>out;const cJSON*e;cJSON_ArrayForEach(e,j){Entry v{str(e,"path"),str(e,"hash"),"",1,0};if(!root_allowed(v.path) || v.hash.size()!=64 || out.size()>=2048){cJSON_Delete(j);fail("Registo de plugins invalido.");}out.push_back(v);}cJSON_Delete(j);return out;}
void save_owned(const std::vector<Entry>&entries){cJSON*j=cJSON_CreateArray();for(auto&e:entries){auto*v=cJSON_CreateObject();cJSON_AddStringToObject(v,"path",e.path.c_str());cJSON_AddStringToObject(v,"hash",e.hash.c_str());cJSON_AddItemToArray(j,v);}char*text=cJSON_PrintUnformatted(j);if(!text){cJSON_Delete(j);fail("Sem memoria para registar plugins.");}const auto target=base+"/owned-plugins.json",temp=target+".new",old=target+".previous";FILE*f=fopen(disk(temp).c_str(),"wb");bool ok=f && fwrite(text,1,strlen(text),f)==strlen(text);if(f && fclose(f))ok=false;cJSON_free(text);cJSON_Delete(j);if(!ok)fail("Nao foi possivel registar o plugin novo.");if(kind(old))remove_file(old);bool had=kind(target);if(had)move(target,old);try{move(temp,target);}catch(...){if(had)move(old,target);throw;}if(had)remove_file(old);}
}
extern "C" bool safety_backup_track_plugin(const char*path,char*message,size_t cap){std::lock_guard<std::mutex>guard(gate);try{if(!path || !root_allowed(path))return true;auto s=load(base+"/baseline");if(preserved_plugin(s,path))return true;auto entries=owned_plugins();auto hash=hash_file(path);bool found=false;for(auto&e:entries)if(e.path==path){e.hash=hash;found=true;}if(!found)entries.push_back({path,hash,"",1,0});save_owned(entries);return true;}catch(const std::exception&e){if(message && cap)snprintf(message,cap,"%s",e.what());return false;}}
extern "C" bool safety_backup_remove_additions(char*message,size_t cap){std::lock_guard<std::mutex>guard(gate);try{auto s=load(base+"/baseline");auto entries=owned_plugins();for(auto&e:entries){if(preserved_plugin(s,e.path) || !kind(e.path))continue;/* Preserve a later user modification rather than delete an unrecognized version. */if(hash_file(e.path)==e.hash)remove_file(e.path);}snprintf(message,cap,"Plugins acrescentados removidos; anteriores e atualizados mantidos.");return true;}catch(const std::exception&e){snprintf(message,cap,"%s",e.what());return false;}}
extern "C" size_t safety_backup_plan(char lines[][128],size_t capacity){
 std::lock_guard<std::mutex>guard(gate);size_t count=0;try{auto s=load(base+"/baseline");std::vector<Entry>current;for(auto&r:s.roots)walk(r,current);mkdirs(base);FILE*report=fopen(disk(base+"/restore-plan.txt").c_str(),"wb");
 auto add=[&](const std::string&text){if(report)fprintf(report,"%s\n",text.c_str());if(count<capacity)snprintf(lines[count],128,"%s",text.c_str());count++;};
 for(const auto&path:std::vector<std::string>{"ur0:tai/config.txt","ux0:tai/config.txt","ux0:pspemu/seplugins/game.txt","ux0:pspemu/seplugins/vsh.txt","ux0:pspemu/seplugins/pops.txt"}){
 const Entry*old=nullptr;for(auto&e:s.entries)if(e.path==path && e.type==1)old=&e;
 if(old){if(!kind(path) || hash_file(path)!=old->hash)add("Repor configuracao: "+path);}else if(kind(path))add("Retirar configuracao criada: "+path);
 }for(auto&e:owned_plugins())if(!preserved_plugin(s,e.path) && kind(e.path) && hash_file(e.path)==e.hash)add("Remover plugin novo: "+e.path);add("Plugins anteriores e modulos Adrenaline: mantidos.");if(report)fclose(report);if(!count)add("Nenhum ficheiro de plugins alterado.");if(count>capacity && capacity)snprintf(lines[capacity-1],128,"Lista completa: safety-backup/restore-plan.txt");return std::min(count,capacity);
 }catch(const std::exception&e){if(capacity){snprintf(lines[0],128,"%s",e.what());return 1;}return 0;}}
