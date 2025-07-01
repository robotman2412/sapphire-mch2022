
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "esp_err.h"

// Initialize the Sapphire driver.
esp_err_t sapphire_init();
// Do a test thingy.
esp_err_t sapphire_driver_test();

// Install the Sapphire bitstream onto the FPGA.
esp_err_t sapphire_load_bitstream();
