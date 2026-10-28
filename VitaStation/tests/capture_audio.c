/* Export the exact synthesis code used at Vita audio initialization. */
#include "audio_synth.h"
#include <stdio.h>
#include <stdint.h>
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void u32(FILE*f,unsigned v){u16(f,v&65535);u16(f,v>>16);}
int main(int argc,char**argv){
    if(argc!=2)return 1;
    FILE*f=fopen(argv[1],"wb");if(!f)return 1;
    unsigned gap=4800,frames=0;
    for(int s=0;s<5;s++)frames+=audio_synth_frames((SoundEffect)s)+gap;
    fwrite("RIFF",1,4,f);u32(f,36+frames*2);fwrite("WAVEfmt ",1,8,f);
    u32(f,16);u16(f,1);u16(f,1);u32(f,48000);u32(f,96000);u16(f,2);u16(f,16);
    fwrite("data",1,4,f);u32(f,frames*2);
    for(int s=0;s<5;s++){
        for(unsigned i=0;i<audio_synth_frames((SoundEffect)s);i++)u16(f,(uint16_t)audio_synth_sample((SoundEffect)s,i));
        for(unsigned i=0;i<gap;i++)u16(f,0);
    }
    int error=ferror(f);if(fclose(f))error=1;return error?1:0;
}
