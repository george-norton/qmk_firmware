# Scylla

A modern, low-profile split ergonomic keyboard

* Keyboard Maintainer: [Bastard Keyboards](https://github.com/Bastardkb/)
* Hardware Supported: Splinky v3, Splinktegrated
* Hardware Availability: [Bastard Keyboards](https://bastardkb.com/)

## Building the firmware

The template is:

```shell
qmk compile -kb bastardkb/scylla -km {KEYMAP}
```

| default                                             |
| --------------------------------------------------- |
| `qmk compile -kb bastardkb/scylla -km default`      |

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

See the [keyboard build instructions](https://docs.bastardkb.com)

### Legacy hardware

For older configurations, including `blackpill`, `v1` shield versions, `elitec` and `splinky_2` support, please see the archived tree at commit [`f0ffdc3b6d`](https://github.com/Bastardkb/bastardkb-qmk/tree/f0ffdc3b6d34b1d0e72474bc2d2296399871d5b9/keyboards/bastardkb/scylla).
