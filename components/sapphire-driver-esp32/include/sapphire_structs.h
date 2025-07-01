
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Does nothing, successfully.
#define SAPPHIRE_CMD_NOP           0
// Get status information about the GPU.
#define SAPPHIRE_CMD_STATUS        1
// Get GPU hardware description structure.
#define SAPPHIRE_CMD_DESC          2
// Clear pending interrupts.
#define SAPPHIRE_CMD_IRQ_CLEAR     3
// Select enabled interrupts.
#define SAPPHIRE_CMD_IRQ_ENABLE    4
// Set up DMA for reading.
#define SAPPHIRE_CMD_READ_DMA      8
// Receive DMA read data.
#define SAPPHIRE_CMD_READ_PAYLOAD  9
// Set up DMA for writing.
#define SAPPHIRE_CMD_WRITE_DMA     10
// Send DMA write data.
#define SAPPHIRE_CMD_WRITE_PAYLOAD 11

// DMA setup completed; data transfer may begin.
#define SAPPHIRE_IRQ_DMA_READY   (1u << 0)
// DMA setup or data transfer error.
#define SAPPHIRE_IRQ_DMA_ERROR   (1u << 1)
// An unknown command was specified.
#define SAPPHIRE_IRQ_CMD_ERROR   (1u << 2)
// A command was provided unsupported parameters.
#define SAPPHIRE_IRQ_PARAM_ERROR (1u << 3)

// The GPU uses 64-bit pointers instead of 32-bit ones.
#define SAPPHIRE_REQF_IS_64BIT (1u << 0)
// Bitmask of all required features.
#define SAPPHIRE_REQF_ALL      SAPPHIRE_REQF_IS_64BIT

// The GPU has support for 3D rendering.
#define SAPPHIRE_OPTF_HAS_3D   (1u << 0)
// The GPU has support for vertex coloring.
#define SAPPHIRE_OPTF_HAS_VCOL (1u << 1)

// Status register struct.
typedef struct {
    // Which intterrupts are asserted, regardless of enable.
    uint32_t irq_state;
    // Which interrupts are currently enabled.
    uint32_t irq_enable;
} sapphire_statreg_t;

// GPU hardware description struct.
typedef struct {
    // SEMVER revision of the GPU.
    uint8_t  gpu_major, gpu_minor, gpu_patch;
    // Number of scanout engines.
    uint8_t  scanout_count;
    // Bitset of implemented interrupts.
    uint32_t irq_impl;
    // Byte size of the GPU's RAM.
    uint64_t ram_size;
    // Bitset of features that the driver is required to support.
    uint32_t required_features;
    // Bitset of features that the driver may optionally support.
    uint32_t optional_features;
    // Number of bits used for pixel coordinates.
    uint8_t  coord_bits;
} sapphire_hwdesc_t;
