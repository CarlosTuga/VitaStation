#include "bootstrap_exit.h"
#include <stdio.h>
#include <string.h>
bool bootstrap_record_components(const char*marker){
 if(!marker || !*marker || strlen(marker)>220)return false;
 char temp[256],previous[256],data[10];snprintf(temp,sizeof(temp),"%s.new",marker);snprintf(previous,sizeof(previous),"%s.previous",marker);
 FILE*f=fopen(marker,"rb");bool had=f!=NULL;if(f){size_t n=fread(data,1,sizeof(data),f);bool valid=n==9 && !memcmp(data,"prepared\n",9) && !ferror(f);fclose(f);if(valid)return true;}
 f=fopen(temp,"wb");bool ok=f && fputs("prepared\n",f)>=0;if(f && fclose(f))ok=false;if(!ok){remove(temp);return false;}
 if(had){remove(previous);if(rename(marker,previous)){remove(temp);return false;}}
 if(rename(temp,marker)){if(had)rename(previous,marker);remove(temp);return false;}
 return true;
}
