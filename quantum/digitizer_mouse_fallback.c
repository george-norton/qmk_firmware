#if defined(POINTING_DEVICE_DRIVER_digitizer)


#include <stdlib.h>
#include "digitizer.h"
#include "pointing_device.h"
#include "debug.h"
#include "timer.h"
#include "action.h"

#ifndef DIGITIZER_MOUSE_TAP_TIME
#    define DIGITIZER_MOUSE_TAP_TIME 200
#endif

#ifndef DIGITIZER_MOUSE_TAP_HOLD_TIME
#    define DIGITIZER_MOUSE_TAP_HOLD_TIME 300
#endif

#ifndef DIGITIZER_MOUSE_TAP_PAUSE_TIME
#    define DIGITIZER_MOUSE_TAP_PAUSE_TIME 20
#endif

#ifndef DIGITIZER_MOUSE_TAP_DISTANCE
#    define DIGITIZER_MOUSE_TAP_DISTANCE 15
#endif

#ifndef DIGITIZER_SCROLL_DIVISOR
#    define DIGITIZER_SCROLL_DIVISOR 10
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_TIME
#    define DIGITIZER_MOUSE_SWIPE_TIME 500
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_DISTANCE
#    define DIGITIZER_MOUSE_SWIPE_DISTANCE 300
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_THRESHOLD
#    define DIGITIZER_MOUSE_SWIPE_THRESHOLD 100
#endif

bool                  digitizer_send_mouse_reports = true;
static report_mouse_t mouse_report                 = {};

static report_mouse_t digitizer_get_mouse_report(report_mouse_t _mouse_report);
static uint16_t       digitizer_get_cpi(void);
static void           digitizer_set_cpi(uint16_t cpi);

const pointing_device_driver_t digitizer_pointing_device_driver = {.init = NULL, .get_report = digitizer_get_mouse_report, .get_cpi = digitizer_get_cpi, .set_cpi = digitizer_set_cpi};

typedef enum { NO_GESTURE, DOWN, POSSIBLE_TAP, PAUSE, HOLD, RIGHT_CLICK, MIDDLE_CLICK, SWIPE, UP } gesture_state;

static gesture_state gesture  = NO_GESTURE;
static int           tap_time = 0;

/**
 * @brief Gets the current digitizer mouse report, the pointing device feature will send this is we
 * nave fallen back to mouse mode.
 *
 * @return report_mouse_t
 */
 static report_mouse_t digitizer_get_mouse_report(report_mouse_t _mouse_report) {
    report_mouse_t report = mouse_report;
    // Retain the button state, but drop any motion.
    memset(&mouse_report, 0, sizeof(report_mouse_t));
    mouse_report.buttons = report.buttons;
    return report;
}

static uint16_t mouse_cpi = 400;

/**
 * @brief Gets the CPI used by the digitizer mouse fallback feature.
 *
 * @return the current CPI value
 */
static uint16_t digitizer_get_cpi(void) {
    return mouse_cpi;
}

/**
 * @brief Sets the CPI used by the digitizer mouse fallback feature.
 *
 *  @param[in] the new CPI value
 */
static void digitizer_set_cpi(uint16_t cpi) {
    mouse_cpi = cpi;
}

/**
 * \brief Internal utility for updating the gesture state once timeouts expire.
 * @return true if a timeout has expired and we should generate a gesture event.
 */
bool update_gesture_state(void) {
    if (digitizer_send_mouse_reports) {
        if (gesture == PAUSE) {
            const uint32_t duration = timer_elapsed32(tap_time);
            if (duration >= DIGITIZER_MOUSE_TAP_PAUSE_TIME) {
                gesture = HOLD;
            }
        }

        if (gesture == POSSIBLE_TAP) {
            const uint32_t duration = timer_elapsed32(tap_time);
            if (duration >= DIGITIZER_MOUSE_TAP_HOLD_TIME) {
                gesture = UP;
                return true;
            }
        }
        if (gesture == RIGHT_CLICK || gesture == MIDDLE_CLICK) {
            gesture = UP;
            return true;
        }
    }
    return false;
}

// We can fallback to reporting as a mouse for hosts which do not implement trackpad support

/**
 * \brief Generate a mouse report from the digitizer report.
 * @param[in] report a new digitizer report
 */
void update_mouse_report(report_digitizer_t *report) {
    static uint16_t last_x   = 0;
    static uint16_t last_y   = 0;
    static bool     last_tip = 0;

    // Some state held to perform basic gesture detection
    static int     contact_start_time = 0;
    static int     contact_start_x    = 0;
    static int     contact_start_y    = 0;
    static uint8_t max_contacts       = 0;

    memset(&mouse_report, 0, sizeof(report_mouse_t));
    int contacts      = 0;
    int last_contacts = 0;

    for (int i = 0; i < DIGITIZER_CONTACT_COUNT; i++) {
        if (report->fingers[i].tip) {
            contacts++;
        }
        if (last_tip) {
            last_contacts++;
        }
    }

    const uint16_t x = report->fingers[0].x * (DIGITIZER_RESOLUTION_X / DIGITIZER_WIDTH_MM) / mouse_cpi;
    const uint16_t y = report->fingers[0].y * (DIGITIZER_RESOLUTION_Y / DIGITIZER_HEIGHT_MM) / mouse_cpi;

    if (last_contacts == 0) {
        if (contacts > 0) {
            contact_start_time = timer_read32();
            contact_start_x    = x;
            contact_start_y    = y;
        }

        if (contacts == 1) {
            if (gesture == POSSIBLE_TAP) {
                gesture = PAUSE;
                tap_time = timer_read32();
            }
            else if (gesture == NO_GESTURE) {
                gesture = DOWN;
            }
        }

        if (gesture == SWIPE) {
            gesture = NO_GESTURE;
        }
        max_contacts = contacts;
    } else {
        max_contacts = MAX(contacts, max_contacts);

        const uint32_t duration = timer_elapsed32(contact_start_time);

        const int32_t distance_x = x - contact_start_x;
        const int32_t distance_y = y - contact_start_y;

        if (gesture == DOWN && (duration > DIGITIZER_MOUSE_TAP_TIME || abs(distance_x) > DIGITIZER_MOUSE_TAP_DISTANCE || abs(distance_y) > DIGITIZER_MOUSE_TAP_DISTANCE)) {
            // Left click
            gesture  = NO_GESTURE;
        }

        switch (contacts) {
            case 0: {
                if (gesture == HOLD) {
                    gesture = NO_GESTURE;
                }

                // Treat short contacts with little travel as a tap
                if (duration < DIGITIZER_MOUSE_TAP_TIME) {
                    // If we tapped quickly, without moving far, send a tap
                    if (max_contacts == 2) {
                        gesture  = RIGHT_CLICK;
                        tap_time = timer_read32();
                    } else if (max_contacts == 3) {
                        gesture  = MIDDLE_CLICK;
                        tap_time = timer_read32();
                    } else if (abs(distance_x) <= DIGITIZER_MOUSE_TAP_DISTANCE && abs(distance_y) <= DIGITIZER_MOUSE_TAP_DISTANCE) {
                        // Left click
                        gesture  = POSSIBLE_TAP;
                        tap_time = timer_read32();
                    }
                }
                break;
            }
            case 1:
                if (gesture == PAUSE && (duration > DIGITIZER_MOUSE_TAP_TIME || abs(distance_x) > DIGITIZER_MOUSE_TAP_DISTANCE || abs(distance_y) > DIGITIZER_MOUSE_TAP_DISTANCE)) {
                    // Left click
                    gesture  = HOLD;
                }
                if (gesture != POSSIBLE_TAP && gesture != UP && gesture != DOWN && gesture != PAUSE && report->fingers[0].tip && last_tip) {
                    mouse_report.x = x - last_x;
                    mouse_report.y = y - last_y;
                }
                break;
            case 2:
                // Scrolling is too fast, so divide the h/v values.
                if (report->fingers[0].tip && last_tip) {
                    static int carry_h = 0;
                    static int carry_v = 0;
                    const int  h       = x - last_x + carry_h;
                    const int  v       = y - last_y + carry_v;

                    carry_h = h % DIGITIZER_SCROLL_DIVISOR;
                    carry_v = v % DIGITIZER_SCROLL_DIVISOR;

                    mouse_report.h = h / DIGITIZER_SCROLL_DIVISOR;
                    mouse_report.v = v / DIGITIZER_SCROLL_DIVISOR;
                }
                break;
            case 3:
                if (gesture != SWIPE && duration < DIGITIZER_MOUSE_SWIPE_TIME) {
                    if (distance_x > 0 && distance_x > DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_y) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                        // Swipe right
                        mouse_report.buttons |= 0x10;
                        gesture = SWIPE;
                    }
                    if (distance_x < 0 && distance_x < -DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_y) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                        // Swipe left
                        mouse_report.buttons |= 0x8;
                        gesture = SWIPE;
                    }
                    if (distance_y < 0 && distance_y < -DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_x) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                        // Swipe up
                        tap_code(KC_LEFT_GUI);
                        gesture = SWIPE;
                    }
                }
                break;
            default:
                break;
                // Do nothing
        }
    }
    if (report->button1 || (max_contacts == 1 && (gesture == HOLD || gesture == POSSIBLE_TAP))) {
        mouse_report.buttons |= 0x1;
    }
    if (report->button2 || gesture == RIGHT_CLICK) {
        mouse_report.buttons |= 0x2;
    }
    if (report->button3 || gesture == MIDDLE_CLICK) {
        mouse_report.buttons |= 0x4;
    }

    if (gesture == UP) {
        gesture = NO_GESTURE;
    }

    last_x   = x;
    last_y   = y;
    last_tip = report->fingers[0].tip;
}
#endif