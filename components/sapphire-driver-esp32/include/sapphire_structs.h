
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
// Read a debug register.
#define SAPPHIRE_CMD_DEBUG_READ    12

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

// Debug latch triggers (updated via SAPPHIRE_CMD_DEBUG_TRIGGERS).
//
// Every debug register implemented is latched when one or more of the following triggers happen.
// The register is initialized to SAPPHIRE_DBG_LATCH_DBGCMD.

// Debug latch: command byte read, and command byte is SAPPHIRE_CMD_DEBUG_READ.
#define SAPPHIRE_DBG_LATCH_DBGCMD  (1 << 0)
// Debug latch: command byte read, and command byte is *not* SAPPHIRE_CMD_DEBUG_READ.
#define SAPPHIRE_DBG_LATCH_CMDBYTE (1 << 1)
// Debug latch: DMA error occurred.
#define SAPPHIRE_DBG_LATCH_DMAERR  (1 << 2)
// Debug latch: DMA data byte transferred.
#define SAPPHIRE_DBG_LATCH_DMABYTE (1 << 3)

// Debug registers (read via SAPPHIRE_CMD_DEBUG_READ).
//
// The DEBUG READ command returns the selected register as a little-endian
// 4-byte (30 significant bit) value, so each register is read into a uint32_t.
// The SAPPHIRE_DBG_* macros below unpack the individual signals from that word;
// the bit layouts mirror the SPI memory controller / DMA bus HDL, where a
// Bundle's first-declared field occupies the least-significant bits.

// Index of each debug register (parameter to SAPPHIRE_CMD_DEBUG_READ).
// SPI mem controller: buffered DMA start address (23 bits).
#define SAPPHIRE_DBG_REG_ADDR   0
// SPI mem controller: command / address shift buffer (23 bits).
#define SAPPHIRE_DBG_REG_BUFFER 1
// SPI mem controller: one-hot FSM state (7 bits).
#define SAPPHIRE_DBG_REG_STATE  2
// DMA setup bus signals plus the write/read stream ready flags.
#define SAPPHIRE_DBG_REG_DMA    3
// DMA write-data stream payload byte.
#define SAPPHIRE_DBG_REG_WDATA  4
// DMA read-data stream payload byte.
#define SAPPHIRE_DBG_REG_RDATA  5

// Reg 0 (ADDR): buffered DMA start address.
#define SAPPHIRE_DBG_ADDR_VALUE(r) ((uint32_t)(r) & 0x7fffffu)

// Reg 1 (BUFFER): command / address shift buffer.
#define SAPPHIRE_DBG_BUFFER_VALUE(r) ((uint32_t)(r) & 0x7fffffu)

// Reg 2 (STATE): one-hot FSM state; test against these masks.
#define SAPPHIRE_DBG_STATE_RESET    (1u << 0)  // Resetting the chip.
#define SAPPHIRE_DBG_STATE_IDLE     (1u << 1)  // Idle; waiting for DMA setup.
#define SAPPHIRE_DBG_STATE_CMD      (1u << 2)  // Sending command.
#define SAPPHIRE_DBG_STATE_PRE_ADDR (1u << 3)  // Command to address dummy cycles.
#define SAPPHIRE_DBG_STATE_ADDR     (1u << 4)  // Sending address.
#define SAPPHIRE_DBG_STATE_PRE_DATA (1u << 5)  // Address to data dummy cycles.
#define SAPPHIRE_DBG_STATE_DATA     (1u << 6)  // Ready to transfer data.

// Reg 3 (DMA): setup bus signals and stream ready flags.
#define SAPPHIRE_DBG_DMA_RDATA_READY(r)    (((uint32_t)(r) >> 0) & 1u)         // rdata stream ready.
#define SAPPHIRE_DBG_DMA_WDATA_READY(r)    (((uint32_t)(r) >> 1) & 1u)         // wdata stream ready.
#define SAPPHIRE_DBG_DMA_SETUP(r)          (((uint32_t)(r) >> 2) & 1u)         // setup requested.
#define SAPPHIRE_DBG_DMA_SETUP_READY(r)    (((uint32_t)(r) >> 3) & 1u)         // ready for setup.
#define SAPPHIRE_DBG_DMA_TEARDOWN(r)       (((uint32_t)(r) >> 4) & 1u)         // teardown requested.
#define SAPPHIRE_DBG_DMA_TEARDOWN_READY(r) (((uint32_t)(r) >> 5) & 1u)         // ready for teardown.
#define SAPPHIRE_DBG_DMA_WRITE(r)          (((uint32_t)(r) >> 6) & 1u)         // access is a write.
#define SAPPHIRE_DBG_DMA_ADDR(r)           (((uint32_t)(r) >> 7) & 0x7fffffu)  // setup start address.

// Reg 4 (WDATA): DMA write-data payload byte.
#define SAPPHIRE_DBG_WDATA_VALUE(r) ((uint32_t)(r) & 0xffu)

// Reg 5 (RDATA): DMA read-data payload byte.
#define SAPPHIRE_DBG_RDATA_VALUE(r) ((uint32_t)(r) & 0xffu)

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
