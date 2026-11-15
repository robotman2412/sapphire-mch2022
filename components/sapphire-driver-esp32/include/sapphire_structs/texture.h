
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

// Texture slot: stored in big-endian.
#define SAPPHIRE_TEXTURE_FLAGS_BIG_ENDIAN (UINT32_C(1) << 0)

// Texture width in pixels.
#define SAPPHIRE_TEXTURE_REG_WIDTH   0x00
// Texture height in pixels.
#define SAPPHIRE_TEXTURE_REG_HEIGHT  0x04
// Texture storage flags.
#define SAPPHIRE_TEXTURE_REG_FLAGS   0x0c
// Texture pixel format (bits per pixel and type).
#define SAPPHIRE_TEXTURE_REG_PIXFMTA 0x10
// Texture pixel format (channel layotu).
#define SAPPHIRE_TEXTURE_REG_PIXFMTB 0x14
// Address stride between each texture slot.
#define SAPPHIRE_TEXTURE_SLOT_STRIDE 0x20

// Sapphire GPU pixel format.
typedef struct {
    // Bits per pixel and type.
    uint32_t a;
    // Channel layout.
    uint32_t b;
} sapphire_pixfmt_t;

// Sapphire GPU texture slot layout.
typedef struct {
    // Texture width in pixels.
    uint32_t          width;
    // Texture height in pixels.
    uint32_t          height;
    // Reserved.
    uint32_t          _resvd0;
    // Texture storage flags.
    uint32_t          flags;
    // Texture pixel format.
    sapphire_pixfmt_t pixfmt;
} sapphire_texture_t;
