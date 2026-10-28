#include "updater.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 UpdateStatus s;
 const char*valid="[{\"draft\":false,\"tag_name\":\"v1.6.2-beta\",\"assets\":[{\"name\":\"VitaStation-1.6.2-beta.vpk\",\"size\":1234,\"digest\":\"sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"browser_download_url\":\"https://github.com/CarlosTuga/VitaStation/releases/download/v1.6.2-beta/VitaStation-1.6.2-beta.vpk\"}]}]";
 assert(update_select(valid,strlen(valid),&s) && s.phase==2 && s.size==1234);
 char bad[2048];strcpy(bad,valid);char*p=strstr(bad,"CarlosTuga");p[0]='X';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 strcpy(bad,valid);p=strstr(bad,"sha256:");p[7]='z';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 strcpy(bad,valid);p=strstr(bad,"v1.6.2");p[5]='0';assert(update_select(bad,strlen(bad),&s) && s.phase==4);
 assert(!update_select("{}",2,&s));assert(!update_select("[",1,&s));assert(update_select("[]",2,&s) && s.phase==4);
 puts("Update selection: newer beta, repository allowlist, digest, old version and invalid JSON passed.");
}