// Copyright 2024 George Norton (@george-norton)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "digitizer.h"
#include "host.h"

enum peacock_keycodes {
    LMB_TOGGLE = QK_USER_0,
    DPI_INC,
    DPI_DEC
};

static uint8_t digitizer_button_state = 0;

const uint16_t PROGMEM etch_a_sketch_combo[] = { KC_LGUI, KC_ENTER, COMBO_END };
combo_t key_combos[] = {
    COMBO(etch_a_sketch_combo, TG(2))
};

#ifdef PEACOCK_ENCODER_BODGE
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(QK_MOUSE_BUTTON_1, QK_MOUSE_BUTTON_2, LT(1, QK_MOUSE_BUTTON_3), DPI_INC, DPI_DEC),
                                                              [1] = LAYOUT(QK_BOOT, QK_RGB_MATRIX_MODE_NEXT, KC_TRNS, QK_RGB_MATRIX_MODE_PREVIOUS, EE_CLR),
                                                              [2] = LAYOUT(LMB_TOGGLE, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),
                                                              [3] = LAYOUT(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS)};
#else
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(KC_MUTE, QK_MOUSE_BUTTON_1, QK_MOUSE_BUTTON_2, LT(1, QK_MOUSE_BUTTON_3), DPI_INC, DPI_DEC, C(KC_C)),
                                                              [1] = LAYOUT(KC_TRNS, QK_BOOT, QK_RGB_MATRIX_MODE_NEXT, KC_TRNS, QK_RGB_MATRIX_MODE_PREVIOUS, EE_CLR, KC_TRNS),
                                                              [2] = LAYOUT(KC_TRNS, LMB_TOGGLE, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),
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

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    switch(get_highest_layer(layer_state|default_layer_state)) {
        case 3:
            rgb_matrix_set_color(INDICATOR_LED, RGB_BLUE);
            break;
        case 2:
            rgb_matrix_set_color(INDICATOR_LED, RGB_RED);
            break;
        case 1:
            rgb_matrix_set_color(INDICATOR_LED, RGB_GREEN);
            break;
        default:
            rgb_matrix_set_color(INDICATOR_LED, RGB_OFF);
            break;
    }
    return false;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    // If we are etch-a-sketching. Ensure the mouse button is released.
    mousekey_clear();
    return state;
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

#ifdef MACOS_TRACKPAD_MODE
#include "pointing_device.h"

report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
    // Let the normal digitizer_task() flow handle sending digitizer reports.
    // This respects the digitizer_send_mouse_reports flag, which ensures proper
    // Microsoft PTP compliance (start as mouse, switch to digitizer after feature request).
    // The digitizer_task_kb() callback below handles updating button state from
    // physical keys, and digitizer_task() will send reports when digitizer_send_mouse_reports
    // is false (either by default for Mac, or after Windows sends the PTP feature request).
    return pointing_device_task_user(mouse_report);
}
#endif

void pointing_device_keycode_handler(uint16_t keycode, bool pressed) {
    if (IS_MOUSEKEY_BUTTON(keycode)) {
        uint8_t button_idx = keycode - QK_MOUSE_BUTTON_1;
        
        // Limit to buttons 1-3 (digitizer supports up to 3 buttons)
        if (button_idx > 2) {
            return;
        }
        
        uint8_t button_mask = 1 << button_idx;
        
        if (pressed) {
            digitizer_button_state |= button_mask;
        } else {
            digitizer_button_state &= ~button_mask;
        }
        
        // Send buttons to both interfaces for cross-platform compatibility:
        // - Digitizer interface: macOS expects buttons from the same interface it uses for pointing
        // - Mouse interface: Linux/other OSes use the Mouse interface for both pointing and buttons
#if defined(DIGITIZER_ENABLE)
        report_digitizer_t digitizer_report = {
            .report_id = REPORT_ID_DIGITIZER,
            .fingers = {},
            .scan_time = 0,
            .contact_count = 0,
            .button1 = (digitizer_button_state & 0x01) ? 1 : 0,
            .button2 = (digitizer_button_state & 0x02) ? 1 : 0,
            .button3 = (digitizer_button_state & 0x04) ? 1 : 0,
            .reserved2 = 0
        };
        host_digitizer_send(&digitizer_report);
#endif
        
        report_mouse_t mouse_report = pointing_device_get_report();
        mouse_report.buttons = pointing_device_handle_buttons(mouse_report.buttons, pressed, button_idx);
        pointing_device_set_report(mouse_report);
        pointing_device_send();
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case LMB_TOGGLE:
        {
            static bool lmb_held = false;
            if (record->event.pressed) {
                lmb_held = !lmb_held;
            }
            if (lmb_held) {
                mousekey_on(QK_MOUSE_BUTTON_1);
            }
            else {
                mousekey_off(QK_MOUSE_BUTTON_1);
            }
            break;
        }
        case DPI_INC:
        {
            uint16_t dpi = pointing_device_get_cpi();
            dpi += 100;
            if (dpi > 1200) dpi=100;
            pointing_device_set_cpi(dpi);
            break;
        }
        case DPI_DEC:
        {
            uint16_t dpi = pointing_device_get_cpi();
            dpi -= 100;
            if (dpi < 100) dpi=1200;
            pointing_device_set_cpi(dpi);
            break;
        }
    }
    return true;
}

bool digitizer_task_kb(digitizer_t *digitizer_state) {
    // Update digitizer button state from physical button state tracked in keycode handler
    digitizer_state->button1 = (digitizer_button_state & 0x01) ? 1 : 0;
    digitizer_state->button2 = (digitizer_button_state & 0x02) ? 1 : 0;
    digitizer_state->button3 = (digitizer_button_state & 0x04) ? 1 : 0;
    
    return digitizer_task_user(digitizer_state);
}
