/* Shared native installer operations; exercised by host I/O mocks. */
static bool safe_name(const char*s){size_t n=strlen(s);if(n<5 || n>94 || s[0]=='.' || strstr(s,".."))return false;for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)s[i]) && s[i]!='_' && s[i]!='-' && s[i]!='.')return false;return !strcmp(s+n-4,".png") || !strcmp(s+n-4,".jpg");}
static bool parse(char*line,char*name,uint64_t*size,uint32_t*crc){unsigned long long bytes;unsigned sum;char extra;if(sscanf(line,"%94s %llu %x %c",name,&bytes,&sum,&extra)!=3 || !safe_name(name) || !bytes || bytes>8*1024*1024)return false;*size=bytes;*crc=sum;return true;}
static bool verify(const char*path,uint64_t expected,uint32_t checksum){
 SceUID fd=sceIoOpen(path,SCE_O_RDONLY,0);if(fd<0)return false;uint64_t bytes=0;uLong crc=crc32(0L,Z_NULL,0);int n;
 while((n=sceIoRead(fd,buffer,sizeof(buffer)))>0){bytes+=(unsigned)n;if(bytes>expected)break;crc=crc32(crc,buffer,(unsigned)n);}
 sceIoClose(fd);return n==0 && bytes==expected && crc==checksum;
}
static SceUID source_blob=-1;
static uint64_t source_offset=0,source_position=0;
static int copy(const char*name,uint64_t expected,uint32_t checksum){
 char source[180],dest[180],temporary[200],backup[240];snprintf(source,sizeof(source),"app0:assets/covers/%s",name);snprintf(dest,sizeof(dest),ROOT "/%s",name);snprintf(temporary,sizeof(temporary),"%s.coverpack.tmp",dest);
 if(verify(dest,expected,checksum))return 1;
 sceIoRemove(temporary); /* Only the incomplete temporary file owned by this installer. */
 bool own_source=source_blob<0;SceUID in=own_source?sceIoOpen(source,SCE_O_RDONLY,0):source_blob;if(in<0)return -1;
 if(!own_source && source_position!=source_offset){if(sceIoLseek(in,(SceOff)source_offset,SCE_SEEK_SET)<0)return -1;source_position=source_offset;}
 SceUID out=sceIoOpen(temporary,SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0777);if(out<0){if(own_source)sceIoClose(in);return -2;}
 uint64_t bytes=0;uLong crc=crc32(0L,Z_NULL,0);int n=0;bool ok=true;
 while(bytes<expected){unsigned wanted=(unsigned)(expected-bytes);if(wanted>sizeof(buffer))wanted=sizeof(buffer);n=sceIoRead(in,buffer,wanted);if(n<=0){ok=false;break;}bytes+=(unsigned)n;if(!own_source)source_position+=(unsigned)n;crc=crc32(crc,buffer,(unsigned)n);int at=0;while(at<n){int wrote=sceIoWrite(out,buffer+at,n-at);if(wrote<=0){ok=false;break;}at+=wrote;}if(!ok)break;}
 if(own_source && ok){unsigned char extra;if(sceIoRead(in,&extra,1)!=0)ok=false;}
 if(bytes!=expected || crc!=checksum)ok=false;
 if(sceIoClose(out)<0)ok=false;
 if(own_source)sceIoClose(in);
 /* Source CRC + acknowledged writes; directory flushes are batched by the caller.
    Existing destination CRC is rechecked on resume instead of rereading every fresh copy. */
 if(!ok){sceIoRemove(temporary);return -3;}
 SceIoStat st;bool had=sceIoGetstat(dest,&st)>=0;
 if(had){snprintf(backup,sizeof(backup),"%s.before-coverpack-%llu.bak",dest,(unsigned long long)sceKernelGetProcessTimeWide());if(sceIoRename(dest,backup)<0){sceIoRemove(temporary);return -4;}}
 if(sceIoRename(temporary,dest)<0){if(had)sceIoRename(backup,dest);return -5;}
 return 0;
}
static bool marker(){const char*path=ROOT "/offline.enabled",*temp=ROOT "/offline.enabled.new";FILE*f=fopen(temp,"wb");if(!f)return false;bool ok=fputs("VitaStation permanent offline covers v1\n",f)>=0;if(fclose(f))ok=false;if(!ok)return false;SceIoStat st;if(sceIoGetstat(path,&st)>=0){sceIoRemove(temp);return true;}return sceIoRename(temp,path)>=0;}
