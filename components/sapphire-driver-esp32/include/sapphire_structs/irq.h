
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025-2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdint.h>

// DMA setup completed; data transfer may begin.
#define SAPPHIRE_IRQ_DMA_READY (1u << 0)
// DMA setup or data transfer error.
#define SAPPHIRE_IRQ_DMA_ERROR (1u << 1)

// Status register struct.
typedef struct {
    // Which intterrupts are asserted, regardless of enable.
    uint32_t irq_state;
    // Which interrupts are currently enabled.
    uint32_t irq_enable;
} sapphire_statreg_t;
