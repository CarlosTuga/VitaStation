#ifndef VITA_AUDIO_H
#define VITA_AUDIO_H

#include <stdbool.h>

typedef enum {
    SND_MOVE = 0,
    SND_SELECT = 1,
    SND_CONFIRM = 2,
    SND_CANCEL = 3,
    SND_MENU_OPEN = 4
} SoundEffect;

void audio_init(void);
void audio_play_sfx(SoundEffect sfx);
void audio_set_music_enabled(bool enable);
void audio_set_sfx_enabled(bool enable);
void audio_shutdown(void);

#endif // VITA_AUDIO_H
