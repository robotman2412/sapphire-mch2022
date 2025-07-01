
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "esp_err.h"

// Run a raw command.
esp_err_t sapphire_do_cmd(uint8_t cmd, void const* param, size_t param_len, void* resp, size_t resp_len);
