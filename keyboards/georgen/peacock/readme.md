# Peacock

![georgen/peacock](https://github.com/george-norton/peacock/blob/main/images/complete.jpg)

Peacock is a QMK powered standalone 7" trackpad. The board features a macropad with 5 switches and 2 encoders and a completly custom designed trackpad. The two parts of the board are connected using ![a VIK interface](https://github.com/sadekbaroudi/vik/blob/master/README.md) so the trackpad could also be used with different VIK compatible keyboards or controllers.

* Keyboard Maintainer: [George Norton](https://github.com/george-norton)
* Hardware Supported: PCBA with integrated RP2040
* Hardware Availability: [Open Source](https://github.com/george-norton/peacock)

Make example for this keyboard (after setting up your build environment):

    make georgen/peacock/v1_01:default
    qmk compile -kb georgen/peacock/v1_01 -km default

Flashing example for this keyboard:

    Put the board into bootloader mode by holding the boot button while connecting the USB cable, bolding boot and shorting the reset jumper or holding SW1 and plugging in the USB cable, then drag and drop the uf2 file onto the drive that mounts.

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## macOS Trackpad Support

The default keymap includes `MACOS_TRACKPAD_MODE` which enables native macOS trackpad functionality. This mode:

* Sends digitizer reports alongside mouse reports for cross-platform compatibility
* Ensures macOS recognizes the device as a trackpad (gesture support, native scrolling, etc.)
* Works on Linux via the standard mouse interface

**Enable/Disable:** Define `MACOS_TRACKPAD_MODE` in your keymap's `config.h`:

```c
#define MACOS_TRACKPAD_MODE  // Enable macOS trackpad mode
```

**How it works:** The implementation uses three function overrides to enable macOS trackpad support:

1. **`pointing_device_task_kb()`** - Sends digitizer reports via the Digitizer HID interface whenever there's pointing activity. macOS uses this interface for native trackpad features (gestures, scrolling, etc.), while other OSes fallback to the Mouse interface. Both interfaces receive reports simultaneously, ensuring compatibility across operating systems.

2. **`pointing_device_keycode_handler()`** - Handles physical button presses and immediately sends digitizer reports with button state changes for responsive macOS button support.

3. **`digitizer_task_kb()`** - Synchronizes physical button state into the digitizer state structure so button presses are reflected in digitizer reports.

**Known limitations:**
* Requires `DIGITIZER_ENABLE` (already enabled in base config)
* Button state is tracked separately for digitizer reports to ensure macOS compatibility
* Limited to 3 buttons (digitizer interface supports up to 3 buttons)

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key or Escape) and plug in the keyboard
* **Physical boot button**: Hold the boot button while plugging in the USB cable, or short the reset pins while holding the boot button.
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available
