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

#ifndef DIGITIZER_MOUSE_TAP_DISTANCE
#    define DIGITIZER_MOUSE_TAP_DISTANCE 15
#endif

#ifndef DIGITIZER_SCROLL_DIVISOR
#    define DIGITIZER_SCROLL_DIVISOR 10
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_TIME
#    define DIGITIZER_MOUSE_SWIPE_TIME 1000
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_DISTANCE
#    define DIGITIZER_MOUSE_SWIPE_DISTANCE 500
#endif

#ifndef DIGITIZER_MOUSE_SWIPE_THRESHOLD
#    define DIGITIZER_MOUSE_SWIPE_THRESHOLD 300
#endif

#ifdef DIGITIZER_REPORT_TAPS_AS_CLICKS
    bool digitizer_taps_as_clicks = true;
#else
    bool digitizer_taps_as_clicks = false;
#endif

bool                  digitizer_send_mouse_reports = true;
static report_mouse_t mouse_report                 = {};

static report_mouse_t digitizer_get_mouse_report(report_mouse_t _mouse_report);
static uint16_t       digitizer_get_cpi(void);
static void           digitizer_set_cpi(uint16_t cpi);

const pointing_device_driver_t digitizer_pointing_device_driver = {.init = NULL, .get_report = digitizer_get_mouse_report, .get_cpi = digitizer_get_cpi, .set_cpi = digitizer_set_cpi};

//typedef enum { NO_GESTURE, DOWN, POSSIBLE_TAP, PAUSE, HOLD, RIGHT_CLICK, MIDDLE_CLICK, SWIPE, UP } gesture_state;

//static gesture_state gesture  = NO_GESTURE;
//static int           tap_time = 0;

/**
 * @brief Gets the current digitizer mouse report, the pointing device feature will send this is we
 * nave fallen back to mouse mode.
 *
 * @return report_mouse_t
 */
 static report_mouse_t digitizer_get_mouse_report(report_mouse_t _mouse_report) {
    if (digitizer_send_mouse_reports) {
        report_mouse_t report = mouse_report;
        // Retain the button state, but drop any motion.
        memset(&mouse_report, 0, sizeof(report_mouse_t));
        mouse_report.buttons = report.buttons;
        return report;
    }
    return _mouse_report;
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

typedef enum {
    None,
    Down,
    MoveScroll,
    Tapped,
    DoubleTapped,
    Drag,
    Swipe,
    Finished
} State;

static State state = None;
static int tap_count = 0;

/**
 * \brief Internal utility for updating the gesture state once timeouts expire.
 * @return true if a timeout has expired and we should generate a gesture event.
 */
bool update_gesture_state(void) {
    return tap_count || state != None;
}

// We can fallback to reporting as a mouse for hosts which do not implement trackpad support
/**
 * \brief Generate a mouse report from the digitizer report.
 * @param[in] report a new digitizer report
 */
void update_mouse_report(report_digitizer_t *report) {
    static int contact_start_time = 0;
    static int contact_start_x    = 0;
    static int contact_start_y    = 0;
    static int tap_contacts       = 0;
    static int last_contacts      = 0;
    static uint16_t last_x        = 0;
    static uint16_t last_y        = 0;
    const uint16_t x              = report->fingers[0].x * (DIGITIZER_RESOLUTION_X / DIGITIZER_WIDTH_MM) / mouse_cpi;
    const uint16_t y              = report->fingers[0].y * (DIGITIZER_RESOLUTION_Y / DIGITIZER_HEIGHT_MM) / mouse_cpi;
    const uint32_t duration       = timer_elapsed32(contact_start_time);
    int contacts                  = 0;

    memset(&mouse_report, 0, sizeof(report_mouse_t));

    for (int i = 0; i < DIGITIZER_CONTACT_COUNT; i++) {
        if (report->fingers[i].tip) {
            contacts++;
        }
    }
    switch (state) {
        case None: {
            if (contacts != 0) {
                state = Down;
                contact_start_time = timer_read32();
                contact_start_x = x;
                contact_start_y = y;
                tap_contacts = contacts;
            }
            break;
        }
        case Down: {
            const uint16_t distance_x = abs(contact_start_x - x);
            const uint16_t distance_y = abs(contact_start_y - y);
            tap_contacts = MAX(contacts, tap_contacts);

            if (contacts == 0) {
                state = Tapped;
                contact_start_time = timer_read32();
            }
            else if (contacts >= 3) {
                state = Swipe;
            }
            else if (   duration > DIGITIZER_MOUSE_TAP_TIME ||
                        distance_x > DIGITIZER_MOUSE_TAP_DISTANCE ||
                        distance_y > DIGITIZER_MOUSE_TAP_DISTANCE) {
                state = MoveScroll;
            }
            break;
        }
        case Drag:
        case MoveScroll: {
            if (contacts == 0) {
                state = None;
            }
            else if (contacts == 1) {
                mouse_report.x = x - last_x;
                mouse_report.y = y - last_y;
            } else if (contacts == 3 && duration < DIGITIZER_MOUSE_SWIPE_TIME) {
                state = Swipe;
            } else {
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
        }
        case DoubleTapped:
        case Tapped: {
            if (contacts == 0 && last_contacts != contacts) {
                tap_count ++;
                state = DoubleTapped;
                contact_start_time = timer_read32();
            }
            else if (duration > DIGITIZER_MOUSE_TAP_TIME) {
                if (contacts > 0 && state == Tapped) {
                    state = Drag;
                } else {
                    state = Finished;
                    tap_count ++;
                }
            }
            break;
        }
        case Swipe: {
            const int32_t distance_x = x - contact_start_x;
            const int32_t distance_y = y - contact_start_y;
            if (contacts == 0) {
                state = None;
            } else if (duration > DIGITIZER_MOUSE_SWIPE_TIME) {
                state = MoveScroll;
            }
            else {
                if (distance_x > DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_y) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                    // Swipe right
                    mouse_report.buttons |= 0x10;
                    state = Finished;
                }
                else if (distance_x < -DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_y) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                    // Swipe left
                    mouse_report.buttons |= 0x8;
                    state = Finished;
                }
                else if (distance_y > DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_x) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                    // Swipe down
                    tap_code(KC_ESC);
                    state = Finished;
                }
                else if (distance_y < -DIGITIZER_MOUSE_SWIPE_DISTANCE && abs(distance_x) < DIGITIZER_MOUSE_SWIPE_THRESHOLD) {
                    // Swipe up
                    tap_code(KC_LEFT_GUI);
                    state = Finished;
                }
            }
            break;
        }
        case Finished:
        {
            if (contacts == 0) {
                state = None;
            }
            break;
        }
    }
    static bool tap = false;
    if (tap_count) {
        tap = !tap;
        if (!tap) {
            tap_count --;
        }
    }
    const bool button_pressed = tap || (state == Drag);
    if (report->button1 || (tap_contacts == 1 && button_pressed)) {
        mouse_report.buttons |= 0x1;
        if (digitizer_taps_as_clicks) report->button1 = 1;
    }
    if (report->button2 || (tap_contacts == 2 && button_pressed)) {
        mouse_report.buttons |= 0x2;
        if (digitizer_taps_as_clicks) report->button2 = 1;
    }
    if (report->button3 || (tap_contacts == 3 && button_pressed)) {
        mouse_report.buttons |= 0x4;
        if (digitizer_taps_as_clicks) report->button3 = 1;
    }
    last_contacts = contacts;
    last_x = x;
    last_y = y;
}
#endif