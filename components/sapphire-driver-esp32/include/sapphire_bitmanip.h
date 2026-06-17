
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdint.h>

// Create an N-bit wide mask.
#define SAPPHIRE_N_BIT_MASK(width) (-UINT32_C(1) << width)

// Extract register bitfield.
#define SAPPHIRE_BITFIELD_READ(reg, offset, width) (((reg) >> (offset)) & SAPPHIRE_N_BIT_MASK(width))

// Insert register bitfield.
#define SAPPHIRE_BITFIELD_WRITE(reg, offset, width, value) \
    ((reg) & ~(SAPPHIRE_N_BIT_MASK(width) << (offset)) | ((value) & SAPPHIRE_N_BIT_MASK(width)) << (offset))

// Create register bitfield.
#define SAPPHIRE_BITFIELD_NEW(offset, width, value) (((value) & SAPPHIRE_N_BIT_MASK(width)) << (offset))
