#include "audio_clip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void audio_clip_free(AudioClip*c){if(c){free(c->storage);memset(c,0,sizeof(*c));}}
bool audio_clip_load(const char*path,AudioClip*c){
 if(!path || !c)return false;memset(c,0,sizeof(*c));FILE*f=fopen(path,"rb");if(!f)return false;
 if(fseek(f,0,SEEK_END)){fclose(f);return false;}long bytes=ftell(f);
 if(bytes<=0 || bytes>4*1024*1024 || bytes%4 || fseek(f,0,SEEK_SET)){fclose(f);return false;}
 size_t padded=((size_t)bytes+8191)&~(size_t)8191;c->storage=calloc(1,padded+63);
 if(!c->storage){fclose(f);return false;}c->samples=(int16_t*)(((uintptr_t)c->storage+63)&~(uintptr_t)63);
 bool ok=fread(c->samples,1,bytes,f)==(size_t)bytes;if(fclose(f))ok=false;
 if(!ok){audio_clip_free(c);return false;}c->frames=(size_t)bytes/4;c->padded_frames=padded/4;return true;
}
