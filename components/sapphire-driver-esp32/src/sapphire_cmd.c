
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire_cmd.h"
#include <stdlib.h>
#include "sapphire.h"
#include "sapphire_transport.h"

esp_err_t sapphire_cmd_nop() {
    return sapphire_do_cmd(SAPPHIRE_CMD_NOP, NULL, 0, NULL, 0);
}

// Get status information.
esp_err_t sapphire_cmd_status(sapphire_statreg_t* r_statreg) {
    return sapphire_do_cmd(SAPPHIRE_CMD_STATUS, NULL, 0, r_statreg, sizeof(sapphire_statreg_t));
}

// Get the GPU hardware description structure.
esp_err_t sapphire_cmd_desc(sapphire_hwdesc_t* r_hwdesc) {
    return sapphire_do_cmd(SAPPHIRE_CMD_DESC, NULL, 0, r_hwdesc, sizeof(sapphire_hwdesc_t));
}

// Clear pending interrupts
esp_err_t sapphire_cmd_irq_clear(uint32_t p_clear, uint32_t* r_status) {
    uint8_t payload[4];
    payload[0] = (uint8_t)p_clear;
    payload[1] = (uint8_t)(p_clear >> 8);
    payload[2] = (uint8_t)(p_clear >> 16);
    payload[3] = (uint8_t)(p_clear >> 24);
    if (r_status) {
        uint8_t   response[4];
        esp_err_t res = sapphire_do_cmd(SAPPHIRE_CMD_IRQ_CLEAR, payload, sizeof(payload), response, sizeof(response));
        if (res == ESP_OK) {
            *r_status = (uint32_t)response[0] | ((uint32_t)response[1] << 8) | ((uint32_t)response[2] << 16) |
                        ((uint32_t)response[3] << 24);
        }
        return res;
    } else {
        return sapphire_do_cmd(SAPPHIRE_CMD_IRQ_CLEAR, payload, sizeof(payload), NULL, 0);
    }
}

// Select enabled interrupts
esp_err_t sapphire_cmd_irq_enable(uint32_t p_enabled) {
    uint8_t payload[4];
    payload[0] = (uint8_t)p_enabled;
    payload[1] = (uint8_t)(p_enabled >> 8);
    payload[2] = (uint8_t)(p_enabled >> 16);
    payload[3] = (uint8_t)(p_enabled >> 24);
    return sapphire_do_cmd(SAPPHIRE_CMD_IRQ_ENABLE, payload, sizeof(payload), NULL, 0);
}

// Set up DMA for reading
esp_err_t sapphire_cmd_read_dma(uint64_t p_addr) {
    uint8_t payload[8];
    payload[0] = (uint8_t)p_addr;
    payload[1] = (uint8_t)(p_addr >> 8);
    payload[2] = (uint8_t)(p_addr >> 16);
    payload[3] = (uint8_t)(p_addr >> 24);
    payload[4] = (uint8_t)(p_addr >> 32);
    payload[5] = (uint8_t)(p_addr >> 40);
    payload[6] = (uint8_t)(p_addr >> 48);
    payload[7] = (uint8_t)(p_addr >> 56);
    if (sapphire_hwdesc.required_features & SAPPHIRE_REQF_IS_64BIT) {
        return sapphire_do_cmd(SAPPHIRE_CMD_READ_DMA, payload, sizeof(payload), NULL, 0);
    } else {
        return sapphire_do_cmd(SAPPHIRE_CMD_READ_DMA, payload, 4, NULL, 0);
    }
}

// Receive DMA read data
esp_err_t sapphire_cmd_read_payload(void* r_data, size_t r_data_len) {
    return sapphire_do_cmd(SAPPHIRE_CMD_READ_PAYLOAD, NULL, 0, r_data, r_data_len);
}

// Set up DMA for writing
esp_err_t sapphire_cmd_write_dma(uint64_t p_addr) {
    uint8_t payload[8];
    payload[0] = (uint8_t)p_addr;
    payload[1] = (uint8_t)(p_addr >> 8);
    payload[2] = (uint8_t)(p_addr >> 16);
    payload[3] = (uint8_t)(p_addr >> 24);
    payload[4] = (uint8_t)(p_addr >> 32);
    payload[5] = (uint8_t)(p_addr >> 40);
    payload[6] = (uint8_t)(p_addr >> 48);
    payload[7] = (uint8_t)(p_addr >> 56);
    if (sapphire_hwdesc.required_features & SAPPHIRE_REQF_IS_64BIT) {
        return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_DMA, payload, sizeof(payload), NULL, 0);
    } else {
        return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_DMA, payload, 4, NULL, 0);
    }
}

// Send DMA write data
esp_err_t sapphire_cmd_write_payload(void const* p_data, size_t p_data_len) {
    return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_PAYLOAD, p_data, p_data_len, NULL, 0);
}

// Read a debug register.
esp_err_t sapphire_cmd_debug(uint16_t p_register, void* r_data, size_t r_data_len) {
    uint8_t payload[2];
    payload[0] = (uint8_t)p_register;
    payload[1] = (uint8_t)(p_register >> 8);
    return sapphire_do_cmd(SAPPHIRE_CMD_DEBUG_READ, payload, sizeof(payload), r_data, r_data_len);
}

// Select which events latch the debug registers (SAPPHIRE_DBG_LATCH_* bitmask).
esp_err_t sapphire_cmd_debug_triggers(uint8_t p_triggers) {
    return sapphire_do_cmd(SAPPHIRE_CMD_DEBUG_TRIGGERS, &p_triggers, 1, NULL, 0);
}

// Tear down the current DMA transfer.
esp_err_t sapphire_cmd_dma_teardown(void) {
    return sapphire_do_cmd(SAPPHIRE_CMD_DMA_TEARDOWN, NULL, 0, NULL, 0);
}

// Read an I/O register.
esp_err_t sapphire_cmd_ioread(uint32_t p_addr, uint32_t* r_data) {
    uint8_t payload[3];
    payload[0] = (uint8_t)p_addr;
    payload[1] = (uint8_t)(p_addr >> 8);
    payload[2] = (uint8_t)(p_addr >> 16);
    uint8_t   response[4];
    esp_err_t res = sapphire_do_cmd(SAPPHIRE_CMD_IOREAD, payload, sizeof(payload), response, sizeof(response));
    if (res == ESP_OK) {
        *r_data = (uint32_t)response[0] | ((uint32_t)response[1] << 8) | ((uint32_t)response[2] << 16) |
                  ((uint32_t)response[3] << 24);
    }
    return res;
}

// Write an I/O register.
esp_err_t sapphire_cmd_iowrite(uint32_t p_addr, uint32_t p_data) {
    uint8_t payload[7];
    payload[0] = (uint8_t)p_addr;
    payload[1] = (uint8_t)(p_addr >> 8);
    payload[2] = (uint8_t)(p_addr >> 16);
    payload[3] = (uint8_t)p_data;
    payload[4] = (uint8_t)(p_data >> 8);
    payload[5] = (uint8_t)(p_data >> 16);
    payload[6] = (uint8_t)(p_data >> 24);
    return sapphire_do_cmd(SAPPHIRE_CMD_IOWRITE, payload, sizeof(payload), NULL, 0);
}
