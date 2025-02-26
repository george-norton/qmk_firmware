#if defined(POINTING_DEVICE_DRIVER_digitizer)
extern bool digitizer_send_mouse_reports;

__attribute__((weak)) void digitizer_update_mouse_report(report_digitizer_t *report);
__attribute__((weak)) bool digitizer_update_gesture_state(void);
#endif