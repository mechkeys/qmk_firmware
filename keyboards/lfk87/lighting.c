#ifdef ISSI_ENABLE


#include <avr/sfr_defs.h>
#include <avr/timer_avr.h>
#include <avr/wdt.h>
#include "lfk87.h"
#include "issi.h"
#include "TWIlib.h"
#include "lighting.h"
#include "debug.h"
#include "rgblight.h"
#include "audio/audio.h"


extern rgblight_config_t rgblight_config; // Declared in rgblight.c

const uint8_t backlight_pwm_map[BACKLIGHT_LEVELS] = BACKLIGHT_PWM_MAP;

// RGB# to ISSI matrix, this is the same across all revisions
const uint8_t rgb_leds[][3][2] = {
        {{0, 0}, {0, 0}, {0, 0}},
        {{1, 1}, {2, 3}, {2, 4}},   // RGB1/RGB17
        {{2, 1}, {2, 2}, {3, 4}},   // RGB2/RGB18
        {{3, 1}, {3, 2}, {3, 3}},   // RGB3/RGB19
        {{4, 1}, {4, 2}, {4, 3}},   // RGB4/RGB20
        {{5, 1}, {5, 2}, {5, 3}},   // RGB5/RGB21
        {{6, 1}, {6, 2}, {6, 3}},   // RGB6/RGB22
        {{7, 1}, {7, 2}, {7, 3}},   // RGB6/RGB23
        {{8, 1}, {8, 2}, {8, 3}},   // RGB8/RGB24
        {{1, 9}, {1, 8}, {1, 7}},   // RGB9/RGB25
        {{2, 9}, {2, 8}, {2, 7}},   // RGB10/RGB26
        {{3, 9}, {3, 8}, {3, 7}},   // RGB11/RGB27
        {{4, 9}, {4, 8}, {4, 7}},   // RGB12/RGB28
        {{5, 9}, {5, 8}, {5, 7}},   // RGB13/RGB29
        {{6, 9}, {6, 8}, {6, 7}},   // RGB14/RGB30
        {{7, 9}, {7, 8}, {6, 6}},   // RGB15/RGB31
        {{8, 9}, {7, 7}, {7, 6}}    // RGB16/RGB32
    };

    // LFK RevB lighting info
    const uint8_t switch_matrices[] = {0, 1};
    const uint8_t rgb_matrices[] = {6, 7};

    // RGB Map:
    //   27  29  10   9   8   7   6
    // 26                                   5
    // 25                                   4
    // 24                                   3
    //   23  22  21  20  14  15  11   1   2
    const uint8_t rgb_sequence[] = {
        27, 29, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 11, 15, 14, 20, 21, 22, 23, 24, 25, 26
    };

    // Maps switch LEDs from Row/Col to ISSI matrix.
    // Value breakdown:
    //     Bit     | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    //             |   | ISSI Col  |    ISSI Row   |
    //             /   |
    //             Device
    const uint8_t switch_leds[MATRIX_ROWS][MATRIX_COLS] =
    KEYMAP(
      0x19, 0x18,   0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94,   0x93,   0x92, 0x91,
      0x29, 0x28,    0x27,  0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0xA9, 0xA8, 0xA7, 0xA6, 0xA5, 0xA4, 0xA3,   0xA2, 0xA1,
      0x39, 0x38,      0x37,  0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0xB9, 0xB8, 0xB7, 0xB6, 0xB5,     0xB3,
      0x49, 0x48,    0x47,     0x45, 0x44, 0x43, 0x42, 0x41, 0xC9, 0xC8, 0xC7, 0xC6, 0xC5,          0xC4,   0xC2,
      0x59, 0x58,   0x57,  0x56,  0x55,             0x51,                   0xD6, 0xE5, 0xE4,         0xE3, 0xE2, 0xE1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

void set_rgb(uint8_t rgb_led, uint8_t red, uint8_t green, uint8_t blue){
#ifdef RGBLIGHT_ENABLE
    uint8_t matrix = rgb_matrices[0];
    if(rgb_led >= 17){
        matrix = rgb_matrices[1];
        rgb_led -= 16;
    }
    if(rgb_leds[rgb_led][0][1] != 0){
        activateLED(matrix, rgb_leds[rgb_led][0][0], rgb_leds[rgb_led][0][1], red);
    }
    if(rgb_leds[rgb_led][1][1] != 0){
        activateLED(matrix, rgb_leds[rgb_led][1][0], rgb_leds[rgb_led][1][1], green);
    }
    if(rgb_leds[rgb_led][2][1] != 0){
        activateLED(matrix, rgb_leds[rgb_led][2][0], rgb_leds[rgb_led][2][1], blue);
    }
#endif
}

void backlight_set(uint8_t level){
#ifdef BACKLIGHT_ENABLE
    uint8_t pwm_value = 0;
    if(level >= BACKLIGHT_LEVELS){
        level = BACKLIGHT_LEVELS;
    }
    if(level > 0){
        pwm_value = backlight_pwm_map[level-1];
    }
    dprintf("BACKLIGHT_LEVELS: %d\n", BACKLIGHT_LEVELS);
    dprintf("backlight_set level: %d pwm: %d\n", level, pwm_value);
    for(int x = 1; x <= 9; x++){
        for(int y = 1; y <= 9; y++){
            activateLED(switch_matrices[0], x, y, pwm_value);
            activateLED(switch_matrices[1], x, y, pwm_value);
        }
    }
#endif
}

void set_underglow(uint8_t red, uint8_t green, uint8_t blue){
#ifdef RGBLIGHT_ENABLE
    for(uint8_t x = 1; x <= 32; x++){
        set_rgb(x, red, green, blue);
    }
#endif
}


void rgblight_set(void) {
#ifdef RGBLIGHT_ENABLE
    for(uint8_t i = 0; (i < sizeof(rgb_sequence)) && (i < RGBLED_NUM); i++){
        if(rgblight_config.enable){
            set_rgb(rgb_sequence[i], led[i].r, led[i].g, led[i].b);
        }else{
            set_rgb(rgb_sequence[i], 0, 0, 0);
        }
    }
#endif
}

void set_backlight_by_keymap(uint8_t col, uint8_t row){
#ifdef RGBLIGHT_ENABLE
    dprintf("event: %d %d\n", col, row);
    uint8_t lookup_value = switch_leds[row][col];
    uint8_t matrix = switch_matrices[0];
    if(lookup_value & 0x80){
        matrix = switch_matrices[1];
    }
    issi_devices[0]->led_dirty = 1;
    uint8_t led_col = (lookup_value & 0x70) >> 4;
    uint8_t led_row = lookup_value & 0x0F;
    dprintf("LED: %02X, %d %d %d\n", lookup_value, matrix, led_col, led_row);
    activateLED(matrix, led_col, led_row, 255);
#endif
}

void force_issi_refresh(){
    issi_devices[0]->led_dirty = true;
    update_issi(0, true);
    issi_devices[3]->led_dirty = true;
    update_issi(3, true);
}

void led_test(){
#ifdef WATCHDOG_ENABLE
    // This test take a long time to run, disable the WTD until its complete
    wdt_disable();
#endif
    backlight_set(0);
    set_underglow(0, 0, 0);
    force_issi_refresh();
    set_underglow(0, 0, 0);
    for(uint8_t x = 0; x < sizeof(rgb_sequence); x++){
        set_rgb(rgb_sequence[x], 255, 0, 0);
        force_issi_refresh();
        _delay_ms(250);
        set_rgb(rgb_sequence[x], 0, 255, 0);
        force_issi_refresh();
        _delay_ms(250);
        set_rgb(rgb_sequence[x], 0, 0, 255);
        force_issi_refresh();
        _delay_ms(250);
        set_rgb(rgb_sequence[x], 0, 0, 0);
        force_issi_refresh();
    }
#ifdef WATCHDOG_ENABLE
    wdt_enable(WDTO_250MS);
#endif
}

void backlight_init_ports(void){
    dprintf("backlight_init_ports\n");
    issi_init();
}

#endif

