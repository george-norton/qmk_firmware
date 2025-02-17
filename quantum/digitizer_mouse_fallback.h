#if defined(POINTING_DEVICE_DRIVER_digitizer)
extern bool digitizer_send_mouse_reports;

__attribute__((weak)) void update_mouse_report(report_digitizer_t *report);
__attribute__((weak)) bool update_gesture_state(void);
#endif