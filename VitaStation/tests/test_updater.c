#include "updater.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 UpdateStatus s;
 const char*valid="[{\"draft\":false,\"tag_name\":\"v1.6.7-beta\",\"assets\":[{\"name\":\"VitaStation-1.6.7-beta.vpk\",\"size\":1234,\"digest\":\"sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"browser_download_url\":\"https://github.com/CarlosTuga/VitaStation/releases/download/v1.6.7-beta/VitaStation-1.6.7-beta.vpk\"}]}]";
 assert(update_select(valid,strlen(valid),&s) && s.phase==2 && s.size==1234);
 char bad[2048];strcpy(bad,valid);char*p=strstr(bad,"CarlosTuga");p[0]='X';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 strcpy(bad,valid);p=strstr(bad,"sha256:");p[7]='z';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 strcpy(bad,valid);p=strstr(bad,"v1.6.7");p[5]='0';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 assert(!update_select("{}",2,&s));assert(!update_select("[",1,&s));assert(update_select("[]",2,&s) && s.phase==4);

 const char*covers="[{\"draft\":false,\"tag_name\":\"v1.6.0-beta\",\"assets\":[{\"name\":\"Caratulas-de-loja-1.04.vpk\",\"size\":269364411,\"digest\":\"sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"browser_download_url\":\"https://github.com/CarlosTuga/VitaStation/releases/download/v1.6.0-beta/Caratulas-de-loja-1.04.vpk\"}]}]";
 assert(update_select_covers(covers,strlen(covers),&s) && s.size==269364411);
 strcpy(bad,covers);p=strstr(bad,"Caratulas-de-loja-1.04.vpk");p[0]='X';assert(!update_select_covers(bad,strlen(bad),&s));
 strcpy(bad,covers);p=strstr(bad,"sha256:");p[7]='z';assert(!update_select_covers(bad,strlen(bad),&s));
 assert(!update_select_covers("[]",2,&s));assert(!update_select_covers("{}",2,&s));
 puts("Update selection: newer beta, repository allowlist, digest, old version and invalid JSON passed.");
}