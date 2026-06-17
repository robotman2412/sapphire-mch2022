
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdint.h>
#include "sapphire_structs/texture.h"

// Scanout capability: is a CRT controller.
#define SAPPHIRE_SCANOUT_CAP_IS_CRTC     (UINT32_C(1) << 0)
// Scanout capability: is a serial controller.
#define SAPPHIRE_SCANOUT_CAP_IS_SERIAL   (UINT32_C(1) << 1)
// Scanout capability: supports I2C display negotiation.
#define SAPPHIRE_SCANOUT_CAP_NEGOTIATION (UINT32_C(1) << 2)
// Scanout capability: supports sending in-band commands.
#define SAPPHIRE_SCANOUT_CAP_COMMANDS    (UINT32_C(1) << 3)

// Scanout control: enabled (read-write; serial type does not automatically send video if enabled).
#define SAPPHIRE_SCANOUT_CONTROL_ENABLED (UINT32_C(1) << 0)
// Scanout status: display attached (read-only).
#define SAPPHIRE_SCANOUT_STATUS_ATTACHED (UINT32_C(1) << 1)
// Scanout control: trigger one frame (trigger; only with SAPPHIRE_SCANOUT_CAP_IS_SERIAL).
#define SAPPHIRE_SCANOUT_CONTROL_TRIGGER (UINT32_C(1) << 2)
// Scanout control: register select; 0: Command, 1: Data (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_SERIAL).
#define SAPPHIRE_SCANOUT_CONTROL_REGSEL  (UINT32_C(1) << 3)

// Register layout for horizontal or vertical timings for `sapphire_scanout_regs_t`.
// All pixel counts minus one (i.e. resolution=399 -> 400 pixels)
typedef struct {
    // Front porch cycles (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_CRTC).
    volatile uint32_t front;
    // Resolution / video cycles (read-write).
    volatile uint32_t resolution;
    // Back porch cycles (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_CRTC).
    volatile uint32_t back;
    // Sync pulse cycles (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_CRTC).
    volatile uint32_t sync;
} sapphire_crt_timing_regs_t;

// Register layout for scanout engines in the I/O space.
typedef struct {
    // Capabilities (read-only).
    volatile uint32_t          caps;
    // Control and status (read-write).
    volatile uint32_t          control;
    // GPU MMU context ID (read-write; only with SAPPHIRE_OPTF_HAS_MMU).
    volatile uint32_t          mmu_ctx;
    // Source virtual address (read-write).
    volatile uint32_t          fb_addr_lo;
    // Source virtual address (read-write; only with SAPPHIRE_REQF_IS_64BIT).
    volatile uint32_t          fb_addr_hi;
    // Pixel color format (read-write).
    sapphire_pixfmt_regs_t     pixfmt;
    // Horizontal timings (read-write; resolution only for non-CRTC).
    sapphire_crt_timing_regs_t htiming;
    // Vertical timings (read-write; resolution only for non-CRTC).
    sapphire_crt_timing_regs_t vtiming;
    // Command/data passthough port (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_SERIAL).
    volatile uint32_t          serial_data;
} sapphire_scanout_regs_t;
