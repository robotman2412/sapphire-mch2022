
// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

module wrapper (
    input  wire      clk_12m,
    output wire      irq_out_n,

    inout  wire      lcd_cs_n,
    output wire      lcd_rsel,
    input  wire      lcd_mode,
    input  wire      lcd_fsync,
    output wire      lcd_write,
    inout  wire      lcd_rst_n,
    output wire[7:0] lcd_data,

    inout  wire[7:0] pmod,

    output wire      ram_cs_n,
    output wire      ram_sclk,
    inout  wire[3:0] ram_qio,

    output wire      uart_tx,
    input  wire      uart_rx,

    input  wire      spi_ss_n,
    input  wire      spi_sclk,
    input  wire      spi_mosi,
    output wire      spi_miso,

    output wire      led_r,
    output wire      led_g,
    output wire      led_b
);
    assign led_r = 1;
    assign led_g = 1;
    assign led_b = 1;

    wire clk_33m;
    wire pll_lock;

    // icepll -i 12 -o 33: 12 MHz * (87 + 1) / (2 ** 5) = 33 MHz.
    // The VCO runs at 1056 MHz.
    // Pin 35 is the dedicated PLL input, so use PAD rather than CORE.
    SB_PLL40_PAD #(
        .FEEDBACK_PATH("SIMPLE"),
        .DIVR(4'b0000),
        .DIVF(7'b1010111),
        .DIVQ(3'b101),
        .FILTER_RANGE(3'b001)
    ) pll (
        .PACKAGEPIN(clk_12m),
        .PLLOUTGLOBAL(clk_33m),
        .LOCK(pll_lock),
        .RESETB(1'b1),
        .BYPASS(1'b0)
    );

    wire lcd_rst;
    wire lcd_cs;
    assign lcd_rst_n = (lcd_mode && lcd_rst) ? 0 : 'bz;
    assign lcd_cs_n  = (lcd_mode && lcd_cs)  ? 0 : 'bz;
    
    wire[7:0] ram_mosi;
    wire[7:0] ram_mosi_en;
    wire      ram_cs;
    assign ram_cs_n = !ram_cs;
    generate genvar i; for (i = 0; i < 4; i = i + 1) begin
        assign ram_qio[i] = ram_mosi_en[i] ? ram_mosi[i] : 'bz;
    end endgenerate
    wire irq_out;
    assign irq_out_n = !irq_out;
    Top top(
        .io_ramSclk             (ram_sclk),
        .io_ramMosi             (ram_mosi),
        .io_ramMosiEn           (ram_mosi_en),
        .io_ramMiso             (ram_qio),
        .io_ramChipSelect       (ram_cs),
        .io_irqOut              (irq_out),
        .io_slaveChipSelectN    (spi_ss_n),
        .io_slaveSclk           (spi_sclk),
        .io_slaveMiso           (spi_miso),
        .io_slaveMosi           (spi_mosi),
        .io_dispReset           (lcd_rst),
        .io_dispChipSelect      (lcd_cs),
        .io_dispStrobe          (lcd_write),
        .io_dispRegSel          (lcd_rsel),
        .io_dispData            (lcd_data),
        .clk                    (clk_33m),
        .resetn                 (pll_lock)
    );
    
endmodule
