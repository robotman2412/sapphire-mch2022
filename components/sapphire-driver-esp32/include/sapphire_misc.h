
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "esp_err.h"

#ifndef ESP_ERROR_CHECK_RETURN
/**
 * Macro that returns the error code if an error occurs, optionally running extra code.
 * The extra code can be placed after the function thay may error and it will only run if the error occurs.
 *
 * @example
 * esp_err_t my_function()
 * {
 *     // Let's say `read_file()` returns esp_err_t and we want to `close_file()` on error.
 *     ESP_ERROR_CHECK_RETURN(read_file(), close_file())
 *     // If the code page gets here, `read_file()` succeeded.
 *     process_file();
 *     close_file();
 *     return ESP_OK;
 * }
 */
#define ESP_ERROR_CHECK_RETURN(x, ...)     \
    do {                                   \
        esp_err_t err_rc_ = (x);           \
        if (unlikely(err_rc_ != ESP_OK)) { \
            __VA_ARGS__;                   \
            return err_rc_;                \
        }                                  \
    } while (0)
#endif
