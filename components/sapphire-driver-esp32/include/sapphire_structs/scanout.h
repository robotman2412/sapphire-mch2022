
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include <stdint.h>

// Scanout capability: is a CRT controller.
#define SAPPHIRE_SCANOUT_CAP_IS_CRTC         (UINT32_C(1) << 0)
// Scanout capability: is a serial controller.
#define SAPPHIRE_SCANOUT_CAP_IS_SERIAL       (UINT32_C(1) << 1)
// Scanout capability: supports I2C display negotiation.
#define SAPPHIRE_SCANOUT_CAP_NEGOTIATION     (UINT32_C(1) << 2)
// Scanout capability: supports sending in-band commands.
#define SAPPHIRE_SCANOUT_CAP_COMMANDS        (UINT32_C(1) << 3)
// Scanout capability: supports display reset.
#define SAPPHIRE_SCANOUT_CAP_RESET           (UINT32_C(1) << 4)
// Scanout capability: supports pixel format conversion.
// If not supported, the input buffer's format must exactly match the display's format.
// If supported, the pixels' formats are always converted according to SAPPHIRE_SCANOUT_REG_PIXFMT.
#define SAPPHIRE_SCANOUT_CAP_PIXCONV         (UINT32_C(1) << 5)
// Scanout capability: supports partial refresh.
#define SAPPHIRE_SCANOUT_CAP_PARTIAL_REFRESH (UINT32_C(1) << 6)

// Scanout control: enabled (read-write; serial type does not automatically send video if enabled).
#define SAPPHIRE_SCANOUT_CONTROL_ENABLED   (UINT32_C(1) << 0)
// Scanout control: display attached (read-only).
#define SAPPHIRE_SCANOUT_CONTROL_ATTACHED  (UINT32_C(1) << 1)
// Scanout control: trigger one frame (trigger; only with SAPPHIRE_SCANOUT_CAP_IS_SERIAL).
#define SAPPHIRE_SCANOUT_CONTROL_TRIGGER   (UINT32_C(1) << 2)
// Scanout control: register select; 0: Command, 1: Data (read-write; only with SAPPHIRE_SCANOUT_CAP_IS_SERIAL).
#define SAPPHIRE_SCANOUT_CONTROL_REGSEL    (UINT32_C(1) << 3)
// Scanout control: display reset (read-write; active-high).
#define SAPPHIRE_SCANOUT_CONTROL_RESET     (UINT32_C(1) << 4)
// Scanout control: force chip enable.
#define SAPPHIRE_SCANOUT_CONTROL_FORCE_SEL (UINT32_C(1) << 5)

#define SAPPHIRE_CRT_TIMING_REG_FRONT      0x00
#define SAPPHIRE_CRT_TIMING_REG_RESOLUTION 0x04
#define SAPPHIRE_CRT_TIMING_REG_BACK       0x08
#define SAPPHIRE_CRT_TIMING_REG_SYNC       0x0c

#define SAPPHIRE_SCANOUT_REG_CAPS        0x00
#define SAPPHIRE_SCANOUT_REG_CONTROL     0x04
#define SAPPHIRE_SCANOUT_REG_MMU_CTX     0x08
#define SAPPHIRE_SCANOUT_REG_FB_ADDR_LO  0x0c
#define SAPPHIRE_SCANOUT_REG_FB_ADDR_HI  0x10
#define SAPPHIRE_SCANOUT_REG_PIXFMT      0x14
#define SAPPHIRE_SCANOUT_REG_HTIMING     0x1c
#define SAPPHIRE_SCANOUT_REG_VTIMING     0x2c
#define SAPPHIRE_SCANOUT_REG_SERIAL_DATA 0x3c
