
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire.h"
#include <assert.h>
#include <esp_log.h>
#include "bsp/display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"
#include "pax_gfx.h"
#include "pax_text.h"
#include "sapphire_cmd.h"
#include "sapphire_misc.h"
#include "sapphire_structs.h"
#include "sapphire_structs/scanout.h"
#include "sapphire_transport.h"
#include "targets/mch2022/mch2022_hardware.h"

static char const TAG[] = "sapphire";

// Current GPU hardware description structure.
sapphire_hwdesc_t sapphire_hwdesc;

// Initialize the Sapphire driver.
esp_err_t sapphire_init() {
    ESP_ERROR_CHECK_RETURN(sapphire_load_bitstream(), ESP_LOGE(TAG, "Failed to load GPU bitstream"));
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_desc(&sapphire_hwdesc), ESP_LOGE(TAG, "Failed to communicate with GPU"));

    ESP_LOGI(TAG, "Sapphire GPU v%d.%d.%d:", sapphire_hwdesc.gpu_major, sapphire_hwdesc.gpu_minor,
             sapphire_hwdesc.gpu_patch);
    ESP_LOGI(TAG, "Scanout engines:   %d", sapphire_hwdesc.scanout_count);
    ESP_LOGI(TAG, "RAM size:          0x%016llx", sapphire_hwdesc.ram_size);
    ESP_LOGI(TAG, "IRQs implemented:  0x%08x", sapphire_hwdesc.irq_impl);
    ESP_LOGI(TAG, "Required features: 0x%08x", sapphire_hwdesc.required_features);
    ESP_LOGI(TAG, "Optional features: 0x%08x", sapphire_hwdesc.optional_features);
    ESP_LOGI(TAG, "Coordinate bits:   %d", sapphire_hwdesc.coord_bits);

    if (sapphire_hwdesc.required_features & ~SAPPHIRE_REQF_ALL) {
        ESP_LOGE(TAG,
                 "Driver does not support this GPU: Required features mask 0x%08x not satisfied by driver's 0x%08x",
                 sapphire_hwdesc.required_features, SAPPHIRE_REQF_ALL);
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_ERROR_CHECK_RETURN(gpio_set_direction(39, GPIO_MODE_INPUT),
                           ESP_LOGE(TAG, "Failed to set GPU interrupt pin as input"));
    ESP_LOGI(TAG, "GPU driver initialized successfully.");

    return ESP_OK;
}

// Read a span of GPU memory.
esp_err_t sapphire_read_mem(uint64_t addr, void* rdata_, size_t len) {
    uint8_t* rdata = rdata_;
    if (!len) {
        ESP_LOGW(TAG, "Zero-length GPU memory read ignored");
        return ESP_OK;
    }
    if (addr + len < addr || addr + len > sapphire_hwdesc.ram_size) {
        ESP_LOGE(TAG, "GPU memory read at 0x%llx size 0x%zx exceeds RAM size 0x%llx", addr, len,
                 sapphire_hwdesc.ram_size);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_enable(SAPPHIRE_IRQ_DMA_READY));
    while (len) {
        size_t max = len < SAPPHIRE_MAX_TRANSFER_SIZE ? len : SAPPHIRE_MAX_TRANSFER_SIZE;
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_read_dma(addr));
        int64_t lim = esp_timer_get_time() + 10000;
        while (gpio_get_level(39)) {
            if (esp_timer_get_time() > lim) {
                ESP_LOGE(TAG, "GPU DMA setup timed out");
                return ESP_ERR_TIMEOUT;
            }
        }
        uint32_t irq_status;
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_clear(SAPPHIRE_IRQ_DMA_ERROR | SAPPHIRE_IRQ_DMA_READY, &irq_status));
        if (irq_status & SAPPHIRE_IRQ_DMA_ERROR) {
            ESP_LOGE(TAG, "GPU reports DMA setup failure");
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_read_payload(rdata, max));
        if (!gpio_get_level(39)) {
            ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_clear(SAPPHIRE_IRQ_DMA_ERROR, &irq_status));
            ESP_LOGE(TAG, "GPU reports DMA transfer failure");
            sapphire_cmd_dma_teardown();
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_dma_teardown());
        addr  += max;
        len   -= max;
        rdata += max;
    }

    return ESP_OK;
}

// Write a span of GPU memory.
esp_err_t sapphire_write_mem(uint64_t addr, void const* wdata_, size_t len) {
    uint8_t const* wdata = wdata_;
    if (!len) {
        ESP_LOGW(TAG, "Zero-length GPU memory write ignored");
        return ESP_OK;
    }
    if (addr + len < addr || addr + len > sapphire_hwdesc.ram_size) {
        ESP_LOGE(TAG, "GPU memory write at 0x%llx size 0x%zx exceeds RAM size 0x%llx", addr, len,
                 sapphire_hwdesc.ram_size);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_enable(SAPPHIRE_IRQ_DMA_ERROR | SAPPHIRE_IRQ_DMA_READY));
    while (len) {
        size_t max = len < SAPPHIRE_MAX_TRANSFER_SIZE ? len : SAPPHIRE_MAX_TRANSFER_SIZE;
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_write_dma(addr));
        int64_t lim = esp_timer_get_time() + 10000;
        while (gpio_get_level(39)) {
            if (esp_timer_get_time() > lim) {
                ESP_LOGE(TAG, "GPU DMA setup timed out");
                return ESP_ERR_TIMEOUT;
            }
        }
        uint32_t irq_status;
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_clear(SAPPHIRE_IRQ_DMA_ERROR | SAPPHIRE_IRQ_DMA_READY, &irq_status));
        if (irq_status & SAPPHIRE_IRQ_DMA_ERROR) {
            ESP_LOGE(TAG, "GPU reports DMA setup failure");
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_write_payload(wdata, max));
        if (!gpio_get_level(39)) {
            ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_clear(SAPPHIRE_IRQ_DMA_ERROR, &irq_status));
            ESP_LOGE(TAG, "GPU reports DMA transfer failure");
            sapphire_cmd_dma_teardown();
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_dma_teardown());
        addr  += max;
        len   -= max;
        wdata += max;
    }

    return ESP_OK;
}

// Read a single debug register into a 32-bit word; logs and returns 0 on error.
static uint32_t sapphire_dbg_read(uint16_t reg) {
    uint32_t  val = 0;
    esp_err_t res = sapphire_cmd_debug(reg, &val, sizeof(val));
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read debug register %u: %s", reg, esp_err_to_name(res));
        return 0;
    }
    return val;
}

// Human-readable name for the SPI memory controller's one-hot FSM state.
static char const* sapphire_dbg_state_name(uint32_t state) {
    switch (state) {
        case SAPPHIRE_DBG_STATE_RESET:
            return "RESET";
        case SAPPHIRE_DBG_STATE_IDLE:
            return "IDLE";
        case SAPPHIRE_DBG_STATE_CMD:
            return "CMD";
        case SAPPHIRE_DBG_STATE_PRE_ADDR:
            return "PRE_ADDR";
        case SAPPHIRE_DBG_STATE_ADDR:
            return "ADDR";
        case SAPPHIRE_DBG_STATE_PRE_DATA:
            return "PRE_DATA";
        case SAPPHIRE_DBG_STATE_DATA:
            return "DATA";
        default:
            return "<invalid>";
    }
}

// Dump the SPI memory controller's FSM state (least verbose).
void sapphire_dump_state() {
    uint32_t state = sapphire_dbg_read(SAPPHIRE_DBG_REG_STATE);
    ESP_LOGI(TAG, "SPI mem state:  %s (one-hot 0x%02x)", sapphire_dbg_state_name(state), state);
}

// Dump the FSM state plus the DMA bus signals (medium verbosity).
void sapphire_dump_dma() {
    sapphire_dump_state();
    uint32_t dma = sapphire_dbg_read(SAPPHIRE_DBG_REG_DMA);
    ESP_LOGI(TAG, "DMA setup:      %s%s%s%s%s addr=0x%06x", SAPPHIRE_DBG_DMA_WRITE(dma) ? "write " : "read ",
             SAPPHIRE_DBG_DMA_SETUP(dma) ? "setup " : "", SAPPHIRE_DBG_DMA_SETUP_READY(dma) ? "setup_rdy " : "",
             SAPPHIRE_DBG_DMA_TEARDOWN(dma) ? "teardown " : "",
             SAPPHIRE_DBG_DMA_TEARDOWN_READY(dma) ? "teardown_rdy " : "", SAPPHIRE_DBG_DMA_ADDR(dma));
    ESP_LOGI(TAG, "DMA streams:    wdata.ready=%u rdata.ready=%u", SAPPHIRE_DBG_DMA_WDATA_READY(dma),
             SAPPHIRE_DBG_DMA_RDATA_READY(dma));
}

// Dump the complete SPI memory controller debug state (most verbose).
void sapphire_dump_full() {
    sapphire_dump_dma();
    uint32_t addr   = sapphire_dbg_read(SAPPHIRE_DBG_REG_ADDR);
    uint32_t buffer = sapphire_dbg_read(SAPPHIRE_DBG_REG_BUFFER);
    uint32_t wdata  = sapphire_dbg_read(SAPPHIRE_DBG_REG_WDATA);
    uint32_t rdata  = sapphire_dbg_read(SAPPHIRE_DBG_REG_RDATA);
    uint32_t err    = sapphire_dbg_read(SAPPHIRE_DBG_REG_ERR);
    ESP_LOGI(TAG, "SPI mem addr:   0x%06x", SAPPHIRE_DBG_ADDR_VALUE(addr));
    ESP_LOGI(TAG, "SPI mem buffer: 0x%06x", SAPPHIRE_DBG_BUFFER_VALUE(buffer));
    ESP_LOGI(TAG, "DMA payloads:   wdata=0x%02x rdata=0x%02x", SAPPHIRE_DBG_WDATA_VALUE(wdata),
             SAPPHIRE_DBG_RDATA_VALUE(rdata));
    ESP_LOGI(TAG, "DMA err diag:   %s rdata(v=%u r=%u) wdata(v=%u r=%u) spi(busy=%u act_v=%u act_r=%u) read_cap=%u",
             SAPPHIRE_DBG_ERR_IS_WRITE(err) ? "write" : "read", SAPPHIRE_DBG_ERR_RDATA_VALID(err),
             SAPPHIRE_DBG_ERR_RDATA_READY(err), SAPPHIRE_DBG_ERR_WDATA_VALID(err), SAPPHIRE_DBG_ERR_WDATA_READY(err),
             SAPPHIRE_DBG_ERR_SPI_BUSY(err), SAPPHIRE_DBG_ERR_ACT_VALID(err), SAPPHIRE_DBG_ERR_ACT_READY(err),
             SAPPHIRE_DBG_ERR_READ_CAP(err));
}

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t data_len;
} lcd_init_cmd_t;

static const lcd_init_cmd_t init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0xAA, 0XE0}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x67, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x8A, 0x01, 0x78}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},

    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x23}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x43, 0x4C}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xA0}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x00}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x36, 0x36, 0x3A, 0x0C, 0x05, 0x4F, 0X87, 0x3C, 0x08, 0x11, 0x35, 0x19, 0x13, 0x00}, 15},
    /* Negative gamma correction */
    {0xE1, {0x00, 0x09, 0x09, 0x05, 0x13, 0x0A, 0x30, 0x78, 0x43, 0x07, 0x0E, 0x0A, 0x26, 0x2C, 0x1F}, 15},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x08, 0x82, 0x27}, 3},
};

static esp_err_t disp_command_fpga(uint8_t cmd, uint8_t const* data, size_t data_len) {
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_iowrite(0x0100 + SAPPHIRE_SCANOUT_REG_CONTROL, 0));
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_iowrite(0x0100 + SAPPHIRE_SCANOUT_REG_SERIAL_DATA, cmd));
    ESP_ERROR_CHECK_RETURN(
        sapphire_cmd_iowrite(0x0100 + SAPPHIRE_SCANOUT_REG_CONTROL, SAPPHIRE_SCANOUT_CONTROL_REGSEL));
    for (size_t i = 0; i < data_len; i++) {
        ESP_ERROR_CHECK_RETURN(sapphire_cmd_iowrite(0x0100 + SAPPHIRE_SCANOUT_REG_SERIAL_DATA, data[i]));
    }
    return ESP_OK;
}

// The BSP keeps the ESP-LCD panel and panel-IO objects private, so to issue raw
// SPI transactions we mirror just enough of their memory layout to recover the
// underlying SPI device handle and D/C pin. These must stay in sync with esp-idf's
// `esp_lcd` component and the `espressif__esp_lcd_ili9341` driver.

// Mirror of `struct esp_lcd_panel_t` (10 method pointers + a `user_data` pointer;
// only the size matters here).
typedef struct {
    void* method[10];
    void* user_data;
} disp_panel_base_t;

// Mirror of the ILI9341 panel object; we only need the `io` handle that follows the base.
typedef struct {
    disp_panel_base_t         base;
    esp_lcd_panel_io_handle_t io;
} disp_ili9341_panel_t;

// Mirror of `struct esp_lcd_panel_io_t` (5 method pointers; only the size matters here).
typedef struct {
    void* method[5];
} disp_panel_io_base_t;

// Mirror of the SPI panel-IO object, up to and including the fields we read.
typedef struct {
    disp_panel_io_base_t base;
    spi_device_handle_t  spi_dev;
    size_t               spi_trans_max_bytes;
    int                  dc_gpio_num;
    void*                on_color_trans_done;
    void*                user_ctx;
    size_t               queue_size;
    size_t               num_trans_inflight;
    int                  lcd_cmd_bits;
    int                  lcd_param_bits;
    uint8_t              cs_ena_pretrans;
    uint8_t              cs_ena_posttrans;
    struct {
        unsigned int dc_cmd_level   : 1;
        unsigned int dc_data_level  : 1;
        unsigned int dc_param_level : 1;
        unsigned int octal_mode     : 1;
        unsigned int quad_mode      : 1;
    } flags;
} disp_panel_io_spi_t;

// Mirror of `lcd_spi_trans_descriptor_t`: the SPI panel-IO registers a pre-transfer
// callback that does `__containerof(trans, ..., base)` and drives the D/C line from
// the `dc_gpio_level` flag, so wrapping our transaction this way sets D/C for us.
typedef struct {
    spi_transaction_t base;
    struct {
        unsigned int dc_gpio_level    : 1;
        unsigned int en_trans_done_cb : 1;
    } flags;
} disp_spi_trans_t;

static esp_err_t disp_command_normal(uint8_t cmd, uint8_t const* data, size_t data_len) {
    // Recover the raw SPI device handle that the BSP hides inside its ESP-LCD panel.
    esp_lcd_panel_handle_t panel;
    ESP_ERROR_CHECK_RETURN(bsp_display_get_panel(&panel));
    assert(panel != NULL);
    disp_panel_io_spi_t* io = (disp_panel_io_spi_t*)((disp_ili9341_panel_t*)panel)->io;
    assert(io != NULL);
    assert(io->spi_dev != NULL);

    // Hold the bus for the whole command so CS stays asserted across the cmd + data phases.
    ESP_ERROR_CHECK_RETURN(spi_device_acquire_bus(io->spi_dev, portMAX_DELAY));

    // Command phase: D/C at command level. Keep CS active if data bytes follow.
    disp_spi_trans_t trans = {
        .base =
            {
                .user      = io,
                .length    = 8,
                .tx_buffer = &cmd,
                .flags     = data_len ? SPI_TRANS_CS_KEEP_ACTIVE : 0,
            },
        .flags = {.dc_gpio_level = io->flags.dc_cmd_level},
    };
    esp_err_t res = spi_device_polling_transmit(io->spi_dev, &trans.base);

    // Data phase: D/C at parameter level, releasing CS on the final transfer.
    while (res == ESP_OK && data_len) {
        size_t max                 = data_len < 2000 ? data_len : 2000;
        trans.base.length          = 8 * max;
        trans.base.tx_buffer       = data;
        trans.base.flags           = max < data_len ? SPI_TRANS_CS_KEEP_ACTIVE : 0;
        trans.flags.dc_gpio_level  = io->flags.dc_param_level;
        res                        = spi_device_polling_transmit(io->spi_dev, &trans.base);
        data                      += max;
        data_len                  -= max;
    }

    spi_device_release_bus(io->spi_dev);
    return res;
}

// Do a test thingy.
esp_err_t sapphire_driver_test(pax_buf_t* gfx) {
    uint32_t caps;
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_ioread(0x0100 + SAPPHIRE_SCANOUT_REG_CAPS, &caps));
    ESP_LOGI(TAG, "Scanout caps: 0x%08x", caps);

    uint16_t width  = 320;
    uint16_t height = 240;

    // Reset display into FPGA mode.
    gpio_set_level(BSP_LCD_MODE_PIN, 1);
    gpio_set_level(BSP_LCD_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BSP_LCD_RESET_PIN, 1);

    // LCD should now be in a reset state but owned by the FPGA.

    // Init command seq.
    size_t const init_cmds_len = sizeof(init_cmds) / sizeof(lcd_init_cmd_t);
    for (size_t i = 0; i < init_cmds_len; i++) {
        ESP_ERROR_CHECK_RETURN(disp_command_fpga(init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_len));
    }

    // Swap X and Y by setting MV; set BGR bit since the MCH2022 panel is BGR-wired.
    // MADCTL
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x36, (uint8_t[]){0x20 | 0x08}, 1));

    // Sleep out — required before the panel will display anything.
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x11, NULL, 0));
    vTaskDelay(pdMS_TO_TICKS(120));

    // Pixel format: 16 bits/pixel (RGB565) to match the 2-bytes-per-pixel framebuffer.
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x3A, (uint8_t[]){0x55}, 1));

    // DISPON
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x29, NULL, 0));

    // CASET — ILI9341 takes start/end as big-endian 16-bit values (high byte first), end inclusive.
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x2a, (uint8_t[]){0, 0, (width - 1) >> 8, (width - 1) & 0xFF}, 4));

    // RASET
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x2b, (uint8_t[]){0, 0, (height - 1) >> 8, (height - 1) & 0xFF}, 4));

    // Make some image data.
    pax_background(gfx, 0);
    pax_draw_circle(gfx, 0xffff7f00, width * 0.5f, height * 0.5f, 30);
    pax_draw_text(gfx, 0xffffffff, pax_font_sky, 18, 0, 0, "Test text");
    pax_join();

    // RAMWR
    uint8_t const* fb = pax_buf_get_pixels(gfx);
    ESP_ERROR_CHECK_RETURN(disp_command_fpga(0x2c, fb, width * height * 2));

    return ESP_OK;
}
