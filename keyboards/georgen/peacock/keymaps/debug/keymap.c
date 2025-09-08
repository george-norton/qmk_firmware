// Copyright 2024 George Norton (@george-norton)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#ifdef PEACOCK_ENCODER_BODGE
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(QK_MOUSE_BUTTON_1, QK_MOUSE_BUTTON_2, LT(1, QK_MOUSE_BUTTON_3), KC_LGUI, KC_ENTER),
                                                              [1] = LAYOUT(QK_BOOT, QK_RGB_MATRIX_MODE_NEXT, KC_TRNS, QK_RGB_MATRIX_MODE_PREVIOUS, EE_CLR),
                                                              [2] = LAYOUT(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),
                                                              [3] = LAYOUT(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS)};
#else
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(KC_MUTE, QK_MOUSE_BUTTON_1, QK_MOUSE_BUTTON_2, LT(1, QK_MOUSE_BUTTON_3), KC_LGUI, KC_ENTER, C(KC_C)),
                                                              [1] = LAYOUT(KC_TRNS, QK_BOOT, QK_RGB_MATRIX_MODE_NEXT, KC_TRNS, QK_RGB_MATRIX_MODE_PREVIOUS, EE_CLR, KC_TRNS),
                                                              [2] = LAYOUT(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),
                                                              [3] = LAYOUT(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS)};
#endif

const uint8_t INDICATOR_LED = 5;

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU),  ENCODER_CCW_CW(KC_UP, KC_DOWN)  },
    [1] = { ENCODER_CCW_CW(QK_RGB_MATRIX_HUE_DOWN, QK_RGB_MATRIX_HUE_UP),   ENCODER_CCW_CW(QK_RGB_MATRIX_SATURATION_DOWN, QK_RGB_MATRIX_SATURATION_UP)  },
    [2] = { ENCODER_CCW_CW(QK_MOUSE_CURSOR_LEFT, QK_MOUSE_CURSOR_RIGHT),   ENCODER_CCW_CW(QK_MOUSE_CURSOR_UP, QK_MOUSE_CURSOR_DOWN)  },
    [3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),  ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
};
#endif

void keyboard_post_init_user(void) {
    // Customise these values to desired behaviour
    debug_enable=true;
    debug_matrix=true;
    debug_keyboard=true;
    debug_mouse=true;
}

bool shutdown_kb(bool jump_to_bootloader) {
    if (!shutdown_user(jump_to_bootloader)) {
        return false;
    }
    void rgb_matrix_update_pwm_buffers(void);
    if (jump_to_bootloader) {
        rgb_matrix_set_color_all(RGB_RED);
    }
    else {
        rgb_matrix_set_color_all(RGB_OFF);
    }
    rgb_matrix_update_pwm_buffers();
    return true;
}
