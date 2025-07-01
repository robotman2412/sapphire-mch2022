
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire.h"
#include <esp_log.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "sapphire_cmd.h"
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

    ESP_ERROR_CHECK_RETURN(sapphire_cmd_irq_enable(SAPPHIRE_IRQ_DMA_ERROR | SAPPHIRE_IRQ_DMA_READY));
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
        }
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
        }
        len   -= max;
        wdata += max;
    }

    return ESP_OK;
}

// Do a test thingy.
esp_err_t sapphire_driver_test() {
    char const my_data[]             = "This test string will be in GPU memory.";
    char       rbuf[sizeof(my_data)] = {0};
    ESP_LOGI(TAG, "Starting write test");
    ESP_ERROR_CHECK_RETURN(sapphire_write_mem(14, my_data, sizeof(my_data) - 1));
    ESP_LOGI(TAG, "Write test succeeded");
    ESP_ERROR_CHECK_RETURN(sapphire_read_mem(14, rbuf, sizeof(my_data) - 1));
    ESP_LOGI(TAG, "Read test succeeded; read data: %s", rbuf);

    return ESP_OK;
}
