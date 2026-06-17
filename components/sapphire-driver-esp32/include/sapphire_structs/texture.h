
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2026 Julian Scheffers <julian@scheffers.net>

#pragma once

#include "sapphire_bitmanip.h"

// Create channel format.
#define SAPPHIRE_CHFMT_NEW(width, pos) SAPPHIRE_BITFIELD_NEW(0, 3, (width)) | SAPPHIRE_BITFIELD_NEW(3, 5, (pos))
// Get channel format bit width.
#define SAPPHIRE_CHFMT_WIDTH(chfmt)    SAPPHIRE_BITFIELD_READ((chfmt), 0, 3)
// Get channel format bit position.
#define SAPPHIRE_CHFMT_POS(chfmt)      SAPPHIRE_BITFIELD_READ((chfmt), 3, 5)

// Pixel format type: greyscale; channel 0 defines brightness.
#define SAPPHIRE_PIXFMT_TYPE_GREY  0
// Pixel format type: greyscale with alpha; channel 0 defines brightness, channel 3 defines alpha.
#define SAPPHIRE_PIXFMT_TYPE_GREYA 1
// Pixel format type: RGB; channels 0-2 define red, green and blue respectively.
#define SAPPHIRE_PIXFMT_TYPE_RGB   2
// Pixel format type: channels 0-3 define red, green, blue and alpha respectively.
#define SAPPHIRE_PIXFMT_TYPE_RGBA  3

// Create pixel format (part A).
#define SAPPHIRE_PIXFMTB_NEW(bpp, type)       SAPPHIRE_BITFIELD_NEW(0, 6, (bpp)) | SAPPHIRE_BITFIELD_NEW(6, 2, (type))
// Create pixel format (part B for greyscale).
#define SAPPHIRE_PIXFMTA_NEW_GREY(brightness) SAPPHIRE_BITFIELD_NEW(0, 8, (brightness))
// Create pixel format (part B for greyscale with alpha).
#define SAPPHIRE_PIXFMTA_NEW_GREYA(brightness, alpha) \
    SAPPHIRE_BITFIELD_NEW(0, 8, (brightness)) | SAPPHIRE_BITFIELD_NEW(24, 8, (alpha))
// Create pixel format (part B for RGB).
#define SAPPHIRE_PIXFMTA_NEW_RGB(red, green, blue) \
    SAPPHIRE_BITFIELD_NEW(0, 8, (red)) | SAPPHIRE_BITFIELD_NEW(8, 8, (green)) | SAPPHIRE_BITFIELD_NEW(16, 8, (blue))
// Create pixel format (part B for RGBA).
#define SAPPHIRE_PIXFMTA_NEW_RGBA(red, green, blue, alpha)                                                             \
    SAPPHIRE_BITFIELD_NEW(0, 8, (red)) | SAPPHIRE_BITFIELD_NEW(8, 8, (green)) | SAPPHIRE_BITFIELD_NEW(16, 8, (blue)) | \
        SAPPHIRE_BITFIELD_NEW(24, 8, (alpha))

// Get BPP from pixel format part A.
#define SAPPHIRE_PIXFMTB_BPP(pixfmt)        SAPPHIRE_BITFIELD_READ((pixfmt), 0, 6)
// Get type from pixel format part A.
#define SAPPHIRE_PIXFMTB_TYPE(pixfmt)       SAPPHIRE_BITFIELD_READ((pixfmt), 6, 2)
// Get the brightness channel from pixel format part B.
#define SAPPHIRE_PIXFMTA_BRIGHTNESS(pixfmt) SAPPHIRE_BITFIELD_READ((pixfmt), 0, 8)
// Get the red channel from pixel format part B.
#define SAPPHIRE_PIXFMTA_RED(pixfmt)        SAPPHIRE_BITFIELD_READ((pixfmt), 0, 8)
// Get the green channel from pixel format part B.
#define SAPPHIRE_PIXFMTA_GREEN(pixfmt)      SAPPHIRE_BITFIELD_READ((pixfmt), 0, 8)
// Get the blue channel from pixel format part B.
#define SAPPHIRE_PIXFMTA_BLUE(pixfmt)       SAPPHIRE_BITFIELD_READ((pixfmt), 0, 8)
// Get the alpha channel from pixel format part B.
#define SAPPHIRE_PIXFMTA_ALPHA(pixfmt)      SAPPHIRE_BITFIELD_READ((pixfmt), 0, 8)

// Register layout for pixel formats in the I/O space.
typedef struct {
    // Pixel format part A (color channels).
    volatile uint32_t pixfmt_a;
    // Pixel format part B (BPP and type).
    volatile uint32_t pixfmt_b;
} sapphire_pixfmt_regs_t;
