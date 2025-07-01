
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire_cmd.h"
#include "sapphire.h"
#include "sapphire_transport.h"

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
    return sapphire_do_cmd(SAPPHIRE_CMD_DESC, &p_clear, 4, r_status, 4);
}

// Select enabled interrupts
esp_err_t sapphire_cmd_irq_enable(uint32_t p_enabled) {
    return sapphire_do_cmd(SAPPHIRE_CMD_DESC, &p_enabled, 4, NULL, 0);
}

// Set up DMA for reading
esp_err_t sapphire_cmd_read_dma(uint64_t p_addr) {
    if (sapphire_hwdesc.required_features & SAPPHIRE_REQF_IS_64BIT) {
        return sapphire_do_cmd(SAPPHIRE_CMD_READ_DMA, &p_addr, 8, NULL, 0);
    } else {
        return sapphire_do_cmd(SAPPHIRE_CMD_READ_DMA, &p_addr, 4, NULL, 0);
    }
}

// Receive DMA read data
esp_err_t sapphire_cmd_read_payload(void* r_data, size_t r_data_len) {
    return sapphire_do_cmd(SAPPHIRE_CMD_READ_PAYLOAD, NULL, 0, r_data, r_data_len);
}

// Set up DMA for writing
esp_err_t sapphire_cmd_write_dma(uint64_t p_addr) {
    if (sapphire_hwdesc.required_features & SAPPHIRE_REQF_IS_64BIT) {
        return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_DMA, &p_addr, 8, NULL, 0);
    } else {
        return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_DMA, &p_addr, 4, NULL, 0);
    }
}

// Send DMA write data
esp_err_t sapphire_cmd_write_payload(void* p_data, size_t p_data_len) {
    return sapphire_do_cmd(SAPPHIRE_CMD_WRITE_PAYLOAD, p_data, p_data_len, NULL, 0);
}
