
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "esp_err.h"
#include "pax_gfx.h"
#include "sapphire_structs.h"

// Current GPU hardware description structure.
extern sapphire_hwdesc_t sapphire_hwdesc;

// Initialize the Sapphire driver.
esp_err_t sapphire_init();

// Read a span of GPU memory.
esp_err_t sapphire_read_mem(uint64_t addr, void* rdata, size_t len);
// Write a span of GPU memory.
esp_err_t sapphire_write_mem(uint64_t addr, void const* wdata, size_t len);

// Dump the SPI memory controller's FSM state (least verbose).
void sapphire_dump_state();
// Dump the FSM state plus the DMA bus signals (medium verbosity).
void sapphire_dump_dma();
// Dump the complete SPI memory controller debug state (most verbose).
void sapphire_dump_full();

// Do a test thingy.
esp_err_t sapphire_driver_test(pax_buf_t* gfx);
