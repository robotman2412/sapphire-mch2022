
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire_transport.h"
#include <driver/spi_master.h>
#include "bsp/mch2022.h"
#include "ice40.h"
#include "rp2040.h"
#include "sapphire.h"
#include "sapphire_misc.h"

extern uint8_t const bitstream_start[] asm("_binary_bitstream_bin_start");
extern uint8_t const bitstream_end[] asm("_binary_bitstream_bin_end");

esp_err_t get_mch2022_ice40_done(bool* value) {
    RP2040 handle;
    ESP_ERROR_CHECK_RETURN(bsp_mch2022_coprocessor_get_handle(&handle));
    uint16_t buttons;
    ESP_ERROR_CHECK_RETURN(rp2040_read_buttons(&handle, &buttons));
    *value = !((buttons >> 5) & 0x01);
    return ESP_OK;
}

esp_err_t set_mch2022_ice40_reset(bool reset) {
    RP2040 handle;
    ESP_ERROR_CHECK_RETURN(bsp_mch2022_coprocessor_get_handle(&handle));
    ESP_ERROR_CHECK_RETURN(rp2040_set_fpga(&handle, reset));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return ESP_OK;
}

ICE40 ice40 = {
    //  Pins
    .spi_bus   = SPI3_HOST,
    .pin_cs    = 27,
    .pin_int   = 39,
    .pin_done  = -1,
    .pin_reset = -1,

    // Configuration
    .spi_speed_full_duplex = 500000,
    .spi_speed_half_duplex = 500000,
    .spi_speed_turbo       = 500000,
    .spi_input_delay_ns    = 0,
    .spi_max_transfer_size = SAPPHIRE_MAX_TRANSFER_SIZE,

    // External pin handlers
    .get_done  = get_mch2022_ice40_done,
    .set_reset = set_mch2022_ice40_reset,
};

// Install the Sapphire bitstream onto the FPGA.
esp_err_t sapphire_load_bitstream() {
    ESP_ERROR_CHECK_RETURN(ice40_init(&ice40));
    esp_err_t res = ice40_load_bitstream(&ice40, bitstream_start, bitstream_end - bitstream_start);
    esp_rom_delay_us(100);
    return res;
}

// Run a raw command.
esp_err_t sapphire_do_cmd(uint8_t cmd, void const* param, size_t param_len, void* resp, size_t resp_len) {
    static uint8_t xfer_buf[SAPPHIRE_MAX_TRANSFER_SIZE];
    if (param_len > SAPPHIRE_MAX_TRANSFER_SIZE - 1 || resp_len > SAPPHIRE_MAX_TRANSFER_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    xfer_buf[0] = cmd;
    memcpy(xfer_buf + 1, param, param_len);
    ESP_ERROR_CHECK_RETURN(ice40_send(&ice40, xfer_buf, param_len + 1));
    esp_rom_delay_us(100);
    if (resp_len) {
        esp_err_t res = ice40_receive(&ice40, resp, resp_len);
        esp_rom_delay_us(100);
        return res;
    } else {
        return ESP_OK;
    }
}
