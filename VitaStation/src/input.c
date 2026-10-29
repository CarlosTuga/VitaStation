#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__vita__)
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#endif

#define STICK_THRESHOLD  64    /* desvio (de 128) a partir do qual o analogico conta como direcao */
#define REPEAT_DELAY     18    /* frames ate comecar a repetir */
#define REPEAT_PERIOD    5     /* frames entre repeticoes */
#define TAP_MAX_MOVE     14    /* px: acima disto o toque passa a deslize */
#define TAP_MAX_FRAMES   30
#define SWIPE_STEP_PX    110

static uint32_t prev_buttons = 0;
#if defined(__vita__)
static int hold_frames[4];
static bool touch_down = false;
static bool touch_moved = false;
static int touch_x0, touch_y0, touch_last_x, touch_acc, touch_frames;
#endif

/* Standard PeekBufferPositive exposes physical Vita L/R at 0x100/0x200.
 * Ext2 exposes L1/R1 at 0x400/0x800. Support either explicit layout. */
#if defined(__vita__)
typedef char check_left_trigger[(SCE_CTRL_LTRIGGER==0x100)?1:-1];
typedef char check_right_trigger[(SCE_CTRL_RTRIGGER==0x200)?1:-1];
typedef char check_left_shoulder[(SCE_CTRL_L1==0x400)?1:-1];
typedef char check_right_shoulder[(SCE_CTRL_R1==0x800)?1:-1];
#endif
uint32_t input_map_buttons(uint32_t raw) {
    uint32_t mapped=0;
    static const uint32_t physical[]={0x10,0x40,0x80,0x20,0x4000,0x2000,0x8000,0x1000,0x500,0xA00,0x8,0x1,0x100000,0x200000,0x40000000,0x10000};
    static const uint32_t logical[]={BTN_UP,BTN_DOWN,BTN_LEFT,BTN_RIGHT,BTN_CROSS,BTN_CIRCLE,BTN_SQUARE,BTN_TRIANGLE,BTN_L1,BTN_R1,BTN_START,BTN_SELECT,BTN_VOLUME_UP,BTN_VOLUME_DOWN,BTN_POWER,BTN_PS};
    for(unsigned i=0;i<sizeof(physical)/sizeof(physical[0]);i++)if(raw&physical[i])mapped|=logical[i];
    return mapped;
}

void input_init(void) {
#if defined(__vita__)
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    memset(hold_frames, 0, sizeof(hold_frames));
    touch_down = false;
#endif
    prev_buttons = 0;
}

void input_poll(InputState *state) {
    if (!state) return;
    memset(state, 0, sizeof(InputState));

#if defined(__vita__)
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    /* > 0: so se houve mesmo uma amostra; 0 amostras nao pode gerar "soltou tudo". */
    if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
        uint32_t current = input_map_buttons(pad.buttons);

        state->lx = (int)pad.lx - 128;
        state->ly = (int)pad.ly - 128;
        state->rx = (int)pad.rx - 128;
        state->ry = (int)pad.ry - 128;

        /* O analogico esquerdo comporta-se como o D-pad. */
        if (state->lx < -STICK_THRESHOLD) current |= BTN_LEFT;
        if (state->lx >  STICK_THRESHOLD) current |= BTN_RIGHT;
        if (state->ly < -STICK_THRESHOLD) current |= BTN_UP;
        if (state->ly >  STICK_THRESHOLD) current |= BTN_DOWN;

        /* Repeticao ao manter premido (primeira acao imediata, depois atraso + periodo). */
        static const uint32_t dirs[4] = { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT };
        uint32_t repeat = 0;
        for (int i = 0; i < 4; i++) {
            if (current & dirs[i]) {
                hold_frames[i]++;
                if (hold_frames[i] > REPEAT_DELAY && ((hold_frames[i] - REPEAT_DELAY) % REPEAT_PERIOD) == 0) {
                    repeat |= dirs[i];
                }
            } else {
                hold_frames[i] = 0;
            }
        }

        state->buttons_held = current;
        state->buttons_pressed = (current & (~prev_buttons)) | repeat;
        state->buttons_released = (~current) & prev_buttons;
        prev_buttons = current;
    }

    SceTouchData touch;
    memset(&touch, 0, sizeof(touch));
    if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) > 0 && touch.reportNum > 0) {
        int x = (int)(touch.report[0].x * 960 / 1920);
        int y = (int)(touch.report[0].y * 544 / 1088);
        state->touch_active = true;
        state->touch_x = x;
        state->touch_y = y;
        if (!touch_down) {
            touch_down = true;
            touch_moved = false;
            touch_x0 = x; touch_y0 = y; touch_last_x = x; touch_acc = 0; touch_frames = 0;
        } else {
            touch_frames++;
            touch_acc += x - touch_last_x;
            touch_last_x = x;
            if (abs(x - touch_x0) > TAP_MAX_MOVE || abs(y - touch_y0) > TAP_MAX_MOVE) touch_moved = true;
            if (touch_moved) {
                while (touch_acc <= -SWIPE_STEP_PX) { state->swipe_steps++; touch_acc += SWIPE_STEP_PX; }
                while (touch_acc >=  SWIPE_STEP_PX) { state->swipe_steps--; touch_acc -= SWIPE_STEP_PX; }
            }
        }
    } else if (touch_down) {
        touch_down = false;
        if (!touch_moved && touch_frames < TAP_MAX_FRAMES) {   /* toque curto: acao ao soltar */
            state->touch_pressed = true;
            state->touch_x = touch_x0;
            state->touch_y = touch_y0;
        }
    }
#else
    // Host mock inputs
#endif
}
