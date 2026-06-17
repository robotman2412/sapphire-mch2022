
`include "build/Top.v"

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
    assign led_b = spi_ss_n;
    
    assign pmod[0] = spi_ss_n;
    assign pmod[1] = spi_sclk;
    assign pmod[2] = spi_mosi;
    assign pmod[3] = spi_miso;
    assign pmod[4] = clk_12m;
    
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
        .io_ramSclk         (ram_sclk),
        .io_ramMosi         (ram_mosi),
        .io_ramMosiEn       (ram_mosi_en),
        .io_ramMiso         (ram_qio),
        .io_ramChipSelect   (ram_cs),
        .io_irqOut          (irq_out),
        .io_slaveChipSelect (!spi_ss_n),
        .io_slaveSclk       (spi_sclk),
        .io_slaveMiso       (spi_miso),
        .io_slaveMosi       (spi_mosi),
        .clk                (clk_12m)
    );
    
endmodule
