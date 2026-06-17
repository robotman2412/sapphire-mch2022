
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025-2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdint.h>

// The GPU uses 64-bit pointers instead of 32-bit ones.
#define SAPPHIRE_REQF_IS_64BIT (UINT32_C(1) << 0)
// Bitmask of all required features.
#define SAPPHIRE_REQF_ALL      SAPPHIRE_REQF_IS_64BIT

// The GPU has support for 3D rendering.
#define SAPPHIRE_OPTF_HAS_3D   (UINT32_C(1) << 0)
// The GPU has support for vertex coloring.
#define SAPPHIRE_OPTF_HAS_VCOL (UINT32_C(1) << 1)
// The GPU has a memory-mapping unit.
#define SAPPHIRE_OPTF_HAS_MMU  (UINT32_C(1) << 2)

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
