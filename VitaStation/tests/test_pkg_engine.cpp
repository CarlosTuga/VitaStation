/* Host-only adversarial fixtures. No real filesystem or network access. */
#include "../third_party/usagi/download.cpp"
#include <cassert>
#include <cstring>
#include <iostream>
static unsigned writes=0,mkdirs=0;
uint32_t pkgi_time_msec(){return 0;}
void vs_engine_log(const char*){}
void pkgi_mkdirs(const char*){++mkdirs;}
void pkgi_rm(const char*){}
void pkgi_delete_dir(const std::string&){}
int64_t pkgi_get_size(const char*){return -1;}
bool pkgi_file_exists(const std::string&){return false;}
InodeType pkgi_get_inode_type(const std::string&){return InodeType::NotExist;}
void pkgi_rename(const std::string&,const std::string&){}
void*pkgi_create(const std::string&){return (void*)1;}
void*pkgi_openrw(const char*){return (void*)1;}
void*pkgi_append(const char*){return (void*)1;}
void pkgi_close(void*){}
int64_t pkgi_seek(void*,uint64_t n){return (int64_t)n;}
int pkgi_read(void*,void*,uint32_t){return 0;}
int pkgi_write(void*,const void*,uint32_t n){++writes;return (int)n;}
std::vector<uint8_t>pkgi_load(const std::string&){return {};}
void pkgi_save(const std::string&,const void*,uint32_t){}
std::vector<std::string>pkgi_list_dir_contents(const std::string&){return {};}
struct MemoryHttp:Http{
    std::vector<uint8_t> bytes;size_t offset=0;bool active=false;
    explicit MemoryHttp(std::vector<uint8_t>b):bytes(std::move(b)){}
    void start(const std::string&,uint64_t at)override{offset=(size_t)at;active=true;}
    int64_t read(uint8_t*out,uint64_t n)override{size_t count=std::min<size_t>((size_t)n,bytes.size()-offset);memcpy(out,bytes.data()+offset,count);offset+=count;return (int64_t)count;}
    void abort()override{}
    int get_status()override{return 200;}
    int64_t get_length()override{return bytes.size();}
    explicit operator bool()const override{return active;}
};
static void be32(uint8_t*p,uint32_t n){for(int i=3;i>=0;i--){p[i]=(uint8_t)n;n>>=8;}}
static void be64(uint8_t*p,uint64_t n){for(int i=7;i>=0;i--){p[i]=(uint8_t)n;n>>=8;}}
static void callbacks(Download&d){d.update_progress_cb=[](uint64_t,uint64_t){};d.update_status=[](const std::string&){};d.is_canceled=[](){return false;};}
static void reject_header(std::vector<uint8_t>bytes,const char*message){
    Download d(std::make_unique<MemoryHttp>(std::move(bytes)));callbacks(d);bool rejected=false;
    try{d.pkgi_download("ux0:","UP0000-PCSE00001_00-TESTPACKAGE00001","https://example.org/fixture",nullptr,nullptr);}
    catch(const DownloadError&e){rejected=std::string(e.what()).find(message)!=std::string::npos;}
    assert(rejected);
}
static void reject_path(const std::string&name){
    Download d(std::make_unique<MemoryHttp>(std::vector<uint8_t>{}));callbacks(d);
    d.item_file=nullptr;d.item_index=0;d.index_count=1;d.enc_offset=0;d.enc_size=4096;d.total_size=4096;d.content_type=CONTENT_TYPE_PSV_GAME;
    memset(d.iv,0,sizeof(d.iv));uint8_t key[16]{};aes128_ctr_init(&d.aes,key);
    d.head.resize(32+name.size()+1);be32(d.head.data(),32);be32(d.head.data()+4,name.size()+1);d.head[27]=4;
    memcpy(d.head.data()+32,name.c_str(),name.size()+1);aes128_ctr(&d.aes,d.iv,0,d.head.data(),32);aes128_ctr(&d.aes,d.iv,32,d.head.data()+32,name.size()+1);
    mkdirs=0;bool rejected=false;try{d.download_files();}catch(const DownloadError&){rejected=true;}assert(rejected && !mkdirs);
}
int main(){
    reject_header(std::vector<uint8_t>(256),"wrong pkg header");
    std::vector<uint8_t> h(256);be32(h.data(),0x7f504b47);be32(h.data()+192,0x7f657874);be32(h.data()+20,1);be64(h.data()+24,4096);be64(h.data()+32,UINT64_MAX);
    reject_header(h,"Invalid package dimensions");be64(h.data()+32,256);be32(h.data()+20,UINT32_MAX);reject_header(h,"Invalid package dimensions");
    for(const char*name:{"../escape","a/../../escape","/absolute","ux0:other/file","a\\b","./hidden"})reject_path(name);
    uint8_t in[5]={0x80,0,0,0x10,0},out[16];bool rejected=false;try{lzrc_decompress(out,sizeof(out),in,sizeof(in));}catch(const DownloadError&){rejected=true;}assert(rejected);
    sha256_ctx sha;sha256_init(&sha);sha256_update(&sha,(const uint8_t*)"abc",3);uint8_t digest[32];sha256_finish(&sha,digest);
    const char*expected="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";for(unsigned i=0;i<32;i++)assert(digest[i]==hexvalue(expected[i*2])*16+hexvalue(expected[i*2+1]));
    std::cout<<"PKG hostile headers, traversal paths, decompression bounds and SHA-256 known vector passed.\n";
}
