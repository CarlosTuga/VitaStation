/* Remove only the two exact VitaStation boot plugin entries; preserve the original config. */
static int disable_config(const char*path){
 SceIoStat st;int result=sceIoGetstat(path,&st);if(result==NOT_FOUND)return 0;if(result<0)return result;if(st.st_size>128*1024)return -1;
 FILE*f=fopen(path,"rb");if(!f)return -1;char*input=calloc(1,(size_t)st.st_size+1),*output=calloc(1,(size_t)st.st_size+1);if(!input || !output){free(input);free(output);fclose(f);return -1;}
 bool ok=fread(input,1,st.st_size,f)==(size_t)st.st_size && !memchr(input,0,(size_t)st.st_size);if(fclose(f))ok=false;size_t used=0;bool changed=false;
 if(ok){for(char*at=input;*at;){char*end=strchr(at,'\n');size_t length=end?(size_t)(end-at+1):strlen(at);char*start=at;while(start<at+length && (*start==' ' || *start=='\t'))start++;char*tail=at+length;while(tail>start && (tail[-1]=='\r' || tail[-1]=='\n' || tail[-1]==' ' || tail[-1]=='\t'))tail--;
  const char*a="ur0:tai/VitaStation/VitaStationBoot.suprx",*b="ux0:tai/VitaStation/VitaStationBoot.suprx";size_t trimmed=tail-start;
  bool own=(trimmed==strlen(a) && !memcmp(start,a,trimmed)) || (trimmed==strlen(b) && !memcmp(start,b,trimmed));
  if(own)changed=true;else{memcpy(output+used,at,length);used+=length;}at+=length;
 }}
 if(ok && changed){char temp[256],backup[256];snprintf(temp,sizeof(temp),"%s.vst-remove.new",path);snprintf(backup,sizeof(backup),"%s.vst-remove-%llu.bak",path,(unsigned long long)sceKernelGetProcessTimeWide());f=fopen(temp,"wb");ok=f && fwrite(output,1,used,f)==used;if(f && fclose(f))ok=false;if(ok){ok=sceIoRename(path,backup)>=0;if(ok && sceIoRename(temp,path)<0){sceIoRename(backup,path);ok=false;}}}
 free(input);free(output);return ok?0:-1;
}
