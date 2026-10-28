#include "log.h"
#include <stdio.h>
#include <time.h>
void vs_log(const char *module,const char *message,int code){
    fprintf(stderr,"[%s] %s (%d)\n",module,message,code);
#if defined(__vita__)
    FILE*f=fopen("ux0:data/VitaStation/diagnostic.log","a");
    if(f){long size=ftell(f);if(size<131072)fprintf(f,"%llu [%s] %s (%d)\n",(unsigned long long)time(NULL),module,message,code);fclose(f);}
#endif
}
