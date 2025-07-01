
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "esp_err.h"
#include "sapphire_structs.h"

// Get status information.
esp_err_t sapphire_cmd_status(sapphire_statreg_t* r_statreg);
// Get the GPU hardware description structure.
esp_err_t sapphire_cmd_desc(sapphire_hwdesc_t* r_hwdesc);
// Clear pending interrupts
esp_err_t sapphire_cmd_irq_clear(uint32_t p_clear, uint32_t* r_status);
// Select enabled interrupts
esp_err_t sapphire_cmd_irq_enable(uint32_t p_enabled);
// Set up DMA for reading
esp_err_t sapphire_cmd_read_dma(uint64_t p_addr);
// Receive DMA read data
esp_err_t sapphire_cmd_read_payload(void* r_data, size_t r_data_len);
// Set up DMA for writing
esp_err_t sapphire_cmd_write_dma(uint64_t p_addr);
// Send DMA write data
esp_err_t sapphire_cmd_write_payload(void* p_data, size_t p_data_len);
