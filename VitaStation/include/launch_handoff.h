#ifndef VS_LAUNCH_HANDOFF_H
#define VS_LAUNCH_HANDOFF_H
#include <stdint.h>
/* VitaSDK: any flags value other than 0x20000 opens LiveArea instead. */
#define VS_LAUNCH_PLAY_FLAGS 0x20000
static inline int vs_launch_id(const char*id){if(!id)return 0;for(unsigned i=0;i<9;i++){unsigned char c=id[i];if(!((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9')))return 0;}return !id[9];}
static inline int vs_launch_same(const char*a,const char*b){for(unsigned i=0;i<10;i++)if(a[i]!=b[i])return 0;return 1;}
static inline int vs_launch_pack(unsigned char*out,const char*caller,const char*target,uint64_t now){
 if(!vs_launch_id(caller)||!vs_launch_id(target)||vs_launch_same(caller,target))return 0;
 if(!vs_launch_same(caller,"VTST00001") && !vs_launch_same(caller,"VSTUP0001"))return 0;
 out[0]='V';out[1]='S';out[2]='L';out[3]=1;for(unsigned i=0;i<9;i++){out[4+i]=caller[i];out[13+i]=target[i];}for(unsigned i=0;i<8;i++)out[22+i]=(unsigned char)(now>>(8*i));out[30]=out[31]=0;return 1;
}
static inline int vs_launch_unpack(const unsigned char*in,char*caller,char*target,uint64_t now){
 if(in[0]!='V'||in[1]!='S'||in[2]!='L'||in[3]!=1)return 0;
 uint64_t issued=0;for(unsigned i=0;i<8;i++)issued|=(uint64_t)in[22+i]<<(8*i);
 if(now<issued || now-issued>5000000)return 0;
 for(unsigned i=0;i<9;i++){caller[i]=in[4+i];target[i]=in[13+i];}caller[9]=target[9]=0;unsigned char test[32];return vs_launch_pack(test,caller,target,issued);
}
/* The initial request must be fresh. Once acknowledged, allow safe shutdown
   work to finish, but never launch after the bounded accepted lease expires. */
static inline int vs_launch_wait_unpack(const unsigned char*in,char*caller,char*target,uint64_t accepted,uint64_t now){
 if(now<accepted || now-accepted>180000000)return 0;
 return vs_launch_unpack(in,caller,target,accepted);
}
static inline void vs_launch_uri(char*out,const char*target){const char prefix[]="psgm:play?titleid=";for(unsigned i=0;i<18;i++)out[i]=prefix[i];for(unsigned i=0;i<9;i++)out[18+i]=target[i];out[27]=0;}
#ifdef __cplusplus
extern "C" {
#endif
int shell_handoff_request(const char*caller,const char*target);
#ifdef __cplusplus
}
#endif
#endif
