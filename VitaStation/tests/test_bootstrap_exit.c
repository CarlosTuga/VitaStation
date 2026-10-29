#include "bootstrap_exit.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 BootstrapExit state={VS_BOOT_SCAN,false};assert(!bootstrap_exit_request(&state));assert(!bootstrap_prepare_begin(&state));
 state=(BootstrapExit){VS_BOOT_SCAN,false};assert(bootstrap_prepare_begin(&state));assert(bootstrap_exit_request(&state));bootstrap_work_done(&state);assert(!bootstrap_exit_request(&state));
 state=(BootstrapExit){VS_BOOT_SCAN,false};bootstrap_work_done(&state);assert(!bootstrap_exit_request(&state));
 assert(bootstrap_record_components("prepared-test.done"));assert(bootstrap_record_components("prepared-test.done"));
 FILE*f=fopen("prepared-test.done","rb");assert(f);char data[16]={0};assert(fread(data,1,sizeof(data),f)==9 && !strcmp(data,"prepared\n"));fclose(f);
 assert(!bootstrap_record_components("missing-folder/prepared.done"));assert(!bootstrap_record_components(""));assert(!bootstrap_record_components(NULL));
 f=fopen("prepared-test.done","wb");assert(f);fputs("old invalid state",f);fclose(f);assert(bootstrap_record_components("prepared-test.done"));
 f=fopen("prepared-test.done.previous","rb");assert(f);fclose(f);
 puts("Startup lifecycle: scan exit cannot enter plugin preparation; active plugin preparation finishes; readiness committed without UI poll and old marker preserved.");
}
