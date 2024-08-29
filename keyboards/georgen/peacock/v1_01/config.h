// Copyright 2024 George Norton (@george-norton)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Oh no! I wired up the wrong I2C pins.
// We will sacrifice the encoder buttons.
#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN GP10
#define I2C1_SCL_PIN GP11
