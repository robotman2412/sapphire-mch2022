
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025-2026 Julian Scheffers <julian@scheffers.net>

#pragma once

// Does nothing, successfully.
#define SAPPHIRE_CMD_NOP            0
// Get status information about the GPU.
#define SAPPHIRE_CMD_STATUS         1
// Get GPU hardware description structure.
#define SAPPHIRE_CMD_DESC           2
// Clear pending interrupts.
#define SAPPHIRE_CMD_IRQ_CLEAR      3
// Select enabled interrupts.
#define SAPPHIRE_CMD_IRQ_ENABLE     4
// Set up DMA for reading.
#define SAPPHIRE_CMD_READ_DMA       8
// Receive DMA read data.
#define SAPPHIRE_CMD_READ_PAYLOAD   9
// Set up DMA for writing.
#define SAPPHIRE_CMD_WRITE_DMA      10
// Send DMA write data.
#define SAPPHIRE_CMD_WRITE_PAYLOAD  11
// Read a debug register.
#define SAPPHIRE_CMD_DEBUG_READ     12
// Select which events latch the debug registers.
#define SAPPHIRE_CMD_DEBUG_TRIGGERS 13
// Tear down the current DMA transfer.
#define SAPPHIRE_CMD_DMA_TEARDOWN   14
// Read I/O register.
#define SAPPHIRE_CMD_IOREAD         15
// Write I/O register.
#define SAPPHIRE_CMD_IOWRITE        16
