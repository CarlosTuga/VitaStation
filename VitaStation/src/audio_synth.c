#include "audio_synth.h"
#include <math.h>

unsigned audio_synth_frames(SoundEffect effect) {
    static const unsigned frames[]={1536,2560,4608,3072,6144};
    return (unsigned)effect<5?frames[effect]:0;
}

/* Original synthesized feedback: soft transients, glass harmonics and an airy tail.
 * No console recordings or proprietary audio assets are embedded. */
int16_t audio_synth_sample(SoundEffect effect,unsigned frame) {
    unsigned length=audio_synth_frames(effect);
    if(!length || frame>=length)return 0;
    static const float notes[]={760,1046.5f,659.25f,523.25f,392};
    const float pi=3.14159265358979323846f;
    float t=frame/(float)VS_AUDIO_RATE,u=frame/(float)(length-1);
    float attack=1-expf(-t*1800),release=(1-u)*(1-u);
    float decay=expf(-u*5.0f),frequency=notes[effect];
    float pitch=effect==SND_CANCEL?-.10f:effect==SND_CONFIRM?.12f:.015f;
    float phase=2*pi*frequency*t*(1+pitch*u*.5f);
    float body=sinf(phase)*.58f+sinf(phase*1.5f)*.15f+sinf(phase*2.002f)*.08f;
    float shimmer=sinf(2*pi*frequency*3*t)*expf(-t*155)*.12f;
    uint32_t hash=frame*747796405u+(unsigned)effect*2891336453u;
    hash=(hash^(hash>>16))*2246822519u;hash^=hash>>13;
    float noise=((hash&65535)/32767.5f-1)*expf(-t*650)*.12f;
    float swell=effect==SND_MENU_OPEN?sinf(2*pi*196*t)*.12f*sinf(pi*u):0;
    float sample=(body*decay+shimmer+noise+swell)*attack*release*4800;
    if(sample>32767)sample=32767;
    if(sample<-32768)sample=-32768;
    return (int16_t)sample;
}
