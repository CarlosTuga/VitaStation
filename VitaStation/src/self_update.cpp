/* Complete app-tree ticket shared by the launcher and the independent updater. */
#include "self_update.h"
#include "../third_party/cJSON/cJSON.h"
#include "../third_party/usagi/sha256.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#if defined(__vita__)
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#else
#include <filesystem>
#endif
namespace {
struct File {std::string path,hash;uint64_t size;};
void fail(const char *s){throw std::runtime_error(s);}
bool relative(const std::string&p){return !p.empty() && p.size()<240 && p[0]!='/' && p.find("..") == std::string::npos && p.find_first_of(":\\\r\n\t") == std::string::npos;}
int type(const std::string&p,uint64_t*size=nullptr){
#if defined(__vita__)
 SceIoStat st{};int n=sceIoGetstat(p.c_str(),&st);if(n==(int)0x80010002)return 0;if(n<0)fail("Sem acesso aos ficheiros da atualizacao.");if(size)*size=st.st_size;if(SCE_S_ISDIR(st.st_mode))return 2;if(SCE_S_ISREG(st.st_mode))return 1;fail("Tipo de ficheiro nao permitido.");return 0;
#else
 std::error_code e;auto s=std::filesystem::symlink_status(p,e);if(e==std::errc::no_such_file_or_directory || s.type()==std::filesystem::file_type::not_found)return 0;if(e || std::filesystem::is_symlink(s))fail("Invalid filesystem entry");if(std::filesystem::is_directory(s))return 2;if(!std::filesystem::is_regular_file(s))fail("Invalid file");if(size)*size=std::filesystem::file_size(p);return 1;
#endif
}
void mkdirs(const std::string&p){
#if defined(__vita__)
 for(size_t at=p.find('/');;at=p.find('/',at+1)){auto part=p.substr(0,at);if(!part.empty() && !type(part) && sceIoMkdir(part.c_str(),0777)<0)fail("Nao foi possivel preparar os ficheiros.");if(at==std::string::npos)break;}
#else
 std::filesystem::create_directories(p);
#endif
}
std::vector<std::string>children(const std::string&p){std::vector<std::string>out;
#if defined(__vita__)
 SceUID dir=sceIoDopen(p.c_str());if(dir<0)fail("Nao foi possivel ler a aplicacao.");int n;while(true){SceIoDirent e{};n=sceIoDread(dir,&e);if(n<=0)break;std::string name=e.d_name;if(name!="." && name!="..")out.push_back(name);}sceIoDclose(dir);if(n<0)fail("Leitura incompleta da aplicacao.");
#else
 for(auto&e:std::filesystem::directory_iterator(p))out.push_back(e.path().filename().string());
#endif
 std::sort(out.begin(),out.end());return out;}
std::string hash(const std::string&path,const std::string*copy=nullptr){FILE*in=fopen(path.c_str(),"rb");if(!in)fail("Ficheiro da atualizacao ausente.");FILE*out=nullptr;if(copy){auto at=copy->find_last_of('/');if(at!=std::string::npos)mkdirs(copy->substr(0,at));out=fopen(copy->c_str(),"wb");if(!out){fclose(in);fail("Sem espaco/acesso para a copia de recuperacao.");}}
 sha256_ctx ctx;sha256_init(&ctx);unsigned char buffer[65536];size_t n;bool ok=true;uint64_t total=0;
 while((n=fread(buffer,1,sizeof(buffer),in))){total+=n;if(total>64*1024*1024 || (out && fwrite(buffer,1,n,out)!=n)){ok=false;break;}sha256_update(&ctx,buffer,n);}
 if(ferror(in))ok=false;
 if(fclose(in))ok=false;
 if(out && fclose(out))ok=false;
 if(!ok)fail("Copia incompleta da aplicacao.");
 unsigned char sum[32];sha256_finish(&ctx,sum);char hex[65];for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",sum[i]);return hex;}
void walk(const std::string&root,const std::string&rel,std::vector<File>&files,uint64_t&total,unsigned depth=0){if(depth>24 || files.size()>8192)fail("Atualizacao demasiado grande.");for(auto&name:children(rel.empty()?root:root+"/"+rel)){auto path=rel.empty()?name:rel+"/"+name;if(!relative(path))fail("Caminho de atualizacao invalido.");uint64_t size=0;int t=type(root+"/"+path,&size);if(t==2)walk(root,path,files,total,depth+1);else if(t==1){total+=size;if(size>64*1024*1024 || total>512*1024*1024 || files.size()>=8192)fail("Atualizacao excede limites de seguranca.");files.push_back({path,"",size});}else fail("Ficheiro da atualizacao desapareceu.");}}
std::vector<unsigned char>read(const std::string&p,size_t limit){uint64_t size=0;if(type(p,&size)!=1 || size>limit)fail("Metadados da atualizacao invalidos.");FILE*f=fopen(p.c_str(),"rb");if(!f)fail("Falha a ler metadados.");std::vector<unsigned char>b(size);bool ok=fread(b.data(),1,b.size(),f)==b.size();if(fclose(f))ok=false;if(!ok)fail("Metadados incompletos.");return b;}
uint32_t le(const unsigned char*b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
std::string sfo(const std::vector<unsigned char>&b,const char*key){if(b.size()<20 || le(b.data())!=0x46535000)fail("SFO invalido.");uint32_t k=le(b.data()+8),d=le(b.data()+12),n=le(b.data()+16);if(n>256 || 20+n*16>k || k>d || d>b.size())fail("SFO fora de limites.");for(unsigned i=0;i<n;i++){auto*e=b.data()+20+i*16;size_t ko=k+(e[0]|e[1]<<8),len=le(e+4);uint64_t vo64=(uint64_t)d+le(e+12);if(vo64>b.size())fail("SFO fora de limites.");size_t vo=(size_t)vo64;if(ko>=d || !memchr(b.data()+ko,0,d-ko) || vo>b.size() || len>b.size()-vo)fail("Entrada SFO invalida.");if(!strcmp((const char*)b.data()+ko,key)){auto*end=(const unsigned char*)memchr(b.data()+vo,0,len);if(!end)fail("Valor SFO invalido.");return std::string((const char*)b.data()+vo,(const char*)end);}}return {};}
std::string identity(const std::string&root){auto b=read(root+"/sce_sys/param.sfo",1024*1024);if(sfo(b,"TITLE_ID")!="VTST00001")fail("O pacote nao pertence ao VitaStation.");auto v=sfo(b,"APP_VER");unsigned a=0,c=0;char extra;if(sscanf(v.c_str(),"%u.%u%c",&a,&c,&extra)!=2 || a>99 || c>999)fail("Versao SFO invalida.");uint64_t exe_size=0;auto exe_path=root+"/eboot.bin";if(type(exe_path,&exe_size)!=1 || exe_size<4 || exe_size>64*1024*1024)fail("Executavel VitaStation invalido.");FILE*exe=fopen(exe_path.c_str(),"rb");if(!exe)fail("Executavel VitaStation ausente.");unsigned char magic[4];bool valid=fread(magic,1,4,exe)==4 && !memcmp(magic,"SCE\0",4);if(fclose(exe))valid=false;if(!valid)fail("Executavel VitaStation invalido.");return v;}
const char*field(const cJSON*j,const char*key){auto*v=cJSON_GetObjectItemCaseSensitive(j,key);return cJSON_IsString(v)?v->valuestring:"";}
void sync(){
#if defined(__vita__)
 if(sceIoSync("ux0:",0)<0)fail("Falha a sincronizar a atualizacao.");
#endif
}
}
extern "C" bool self_update_copy(const char*source,const char*destination,char*error,size_t cap){try{if(type(destination))fail("Copia de recuperacao existente; nao sobrescrever.");std::vector<File>files;uint64_t total=0;walk(source,"",files,total);if(files.empty())fail("Aplicacao instalada vazia.");mkdirs(destination);for(auto&f:files){auto dest=std::string(destination)+"/"+f.path;auto sum=hash(std::string(source)+"/"+f.path,&dest);if(hash(dest)!=sum)fail("Copia de recuperacao falhou SHA-256.");}sync();return true;}catch(const std::exception&e){snprintf(error,cap,"%s",e.what());return false;}}
extern "C" bool self_update_prepare(const char*root,const char*ticket,char*error,size_t cap){try{auto version=identity(root);std::vector<File>files;uint64_t total=0;walk(root,"",files,total);for(auto&f:files)f.hash=hash(std::string(root)+"/"+f.path);cJSON*j=cJSON_CreateObject();cJSON_AddNumberToObject(j,"protocol",1);cJSON_AddStringToObject(j,"title_id","VTST00001");cJSON_AddStringToObject(j,"version",version.c_str());auto*entries=cJSON_AddArrayToObject(j,"files");for(auto&f:files){auto*item=cJSON_CreateObject();cJSON_AddStringToObject(item,"path",f.path.c_str());cJSON_AddNumberToObject(item,"size",f.size);cJSON_AddStringToObject(item,"hash",f.hash.c_str());cJSON_AddItemToArray(entries,item);}char*data=cJSON_PrintUnformatted(j);if(!data){cJSON_Delete(j);fail("Sem memoria para a verificacao.");}auto temp=std::string(ticket)+".new";auto slash=temp.find_last_of('/');if(slash!=std::string::npos)mkdirs(temp.substr(0,slash));FILE*f=fopen(temp.c_str(),"wb");bool ok=f && fwrite(data,1,strlen(data),f)==strlen(data);if(f && fclose(f))ok=false;cJSON_free(data);cJSON_Delete(j);if(!ok)fail("Falha ao guardar verificacao da atualizacao.");
#if defined(__vita__)
 sceIoRemove(ticket);if(sceIoRename(temp.c_str(),ticket)<0)fail("Falha ao concluir verificacao.");
#else
 std::error_code ec;std::filesystem::remove(ticket,ec);std::filesystem::rename(temp,ticket);
#endif
 sync();return true;}catch(const std::exception&e){snprintf(error,cap,"%s",e.what());return false;}}
extern "C" bool self_update_verify(const char*root,const char*ticket,char*version,size_t size,char*error,size_t cap){cJSON*j=nullptr;try{auto data=read(ticket,4*1024*1024);j=cJSON_ParseWithLength((const char*)data.data(),data.size());if(!j)fail("Pedido de instalacao invalido.");auto*p=cJSON_GetObjectItemCaseSensitive(j,"protocol"),*list=cJSON_GetObjectItemCaseSensitive(j,"files");if(!cJSON_IsNumber(p) || p->valueint!=1 || strcmp(field(j,"title_id"),"VTST00001") || !cJSON_IsArray(list))fail("Pedido de instalacao nao reconhecido.");auto native=identity(root);if(native!=field(j,"version"))fail("A versao preparada foi alterada.");std::vector<File>actual;uint64_t total=0;walk(root,"",actual,total);if(cJSON_GetArraySize(list)!=(int)actual.size())fail("Ficheiros inesperados ou ausentes na atualizacao.");const cJSON*item;size_t at=0;cJSON_ArrayForEach(item,list){auto path=std::string(field(item,"path")),expected=std::string(field(item,"hash"));auto*n=cJSON_GetObjectItemCaseSensitive(item,"size");if(!relative(path) || path!=actual[at].path || !cJSON_IsNumber(n) || n->valuedouble!=actual[at].size || expected.size()!=64 || hash(std::string(root)+"/"+path)!=expected)fail("Integridade da atualizacao falhou; instalacao bloqueada.");at++;}snprintf(version,size,"%s",native.c_str());cJSON_Delete(j);return true;}catch(const std::exception&e){cJSON_Delete(j);snprintf(error,cap,"%s",e.what());return false;}}
