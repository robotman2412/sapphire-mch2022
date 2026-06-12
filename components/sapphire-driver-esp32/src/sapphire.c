
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire.h"
#include <esp_log.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "sapphire_cmd.h"
#include "sapphire_misc.h"
#include "sapphire_structs.h"
#include "sapphire_transport.h"

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

// Do a test thingy.
esp_err_t sapphire_driver_test() {
    char const     my_data[]             = "This test string will be in GPU memory.";
    // char const     my_data[]             = "A different message for GRAPHICQUE.";
    char           rbuf[sizeof(my_data)] = {0};
    uint32_t const addr                  = 0xcafe;

    ESP_ERROR_CHECK_RETURN(sapphire_cmd_debug_triggers(SAPPHIRE_DBG_LATCH_DMAERR));

    sapphire_statreg_t statreg;
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_status(&statreg));
    ESP_LOGI(TAG, "IRQ state: %08x   IRQ enable: %08x", statreg.irq_state, statreg.irq_enable);

    ESP_LOGI(TAG, "Starting write test");
    esp_err_t res = sapphire_write_mem(addr, my_data, sizeof(my_data) - 1);
    sapphire_dump_full();
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_status(&statreg));
    ESP_LOGI(TAG, "IRQ state: %08x   IRQ enable: %08x", statreg.irq_state, statreg.irq_enable);
    if (res != ESP_OK) return res;
    ESP_LOGI(TAG, "Write test succeeded");

    res = sapphire_read_mem(addr, rbuf, sizeof(my_data) - 1);
    sapphire_dump_full();
    ESP_ERROR_CHECK_RETURN(sapphire_cmd_status(&statreg));
    ESP_LOGI(TAG, "IRQ state: %08x   IRQ enable: %08x", statreg.irq_state, statreg.irq_enable);
    if (res != ESP_OK) return res;
    ESP_LOGI(TAG, "Read test succeeded; read data: %s", rbuf);

    return ESP_OK;
}
