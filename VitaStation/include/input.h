#ifndef VITA_INPUT_H
#define VITA_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#define BTN_VOLUME_UP (1 << 12)
#define BTN_VOLUME_DOWN (1 << 13)
#define BTN_POWER (1 << 14)
#define BTN_PS (1 << 15)

#define BTN_UP        (1 << 0)
#define BTN_DOWN      (1 << 1)
#define BTN_LEFT      (1 << 2)
#define BTN_RIGHT     (1 << 3)
#define BTN_CROSS     (1 << 4)
#define BTN_CIRCLE    (1 << 5)
#define BTN_SQUARE    (1 << 6)
#define BTN_TRIANGLE  (1 << 7)
#define BTN_L1        (1 << 8)
#define BTN_R1        (1 << 9)
#define BTN_START     (1 << 10)
#define BTN_SELECT    (1 << 11)

typedef struct {
    uint32_t buttons_held;
    uint32_t buttons_pressed;
    uint32_t buttons_released;
    int lx, ly; // Left analog stick (-128 to 127)
    int rx, ry; // Right analog stick (-128 to 127)

    // Touchscreen
    bool touch_active;
    int touch_x;
    int touch_y;
    bool touch_pressed;   // toque curto (gerado ao SOLTAR o dedo, sem deslize)
    int  swipe_steps;     // >0 deslize para a esquerda (seguinte), <0 para a direita
} InputState;

uint32_t input_map_buttons(uint32_t raw_buttons);
void input_init(void);
void input_poll(InputState *state);

#endif // VITA_INPUT_H
