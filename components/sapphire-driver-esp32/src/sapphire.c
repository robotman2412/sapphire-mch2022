
// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

#include "sapphire.h"
#include <esp_log.h>
#include "sapphire_structs.h"
#include "sapphire_transport.h"

static char const TAG[] = "sapphire";

// Current GPU hardware description structure.
static sapphire_hwdesc_t hwdesc;

// Initialize the Sapphire driver.
esp_err_t sapphire_init() {
    esp_err_t res = sapphire_do_cmd(2, NULL, 0, &hwdesc, sizeof(hwdesc));
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Sapphire GPU v%d.%d.%d:", hwdesc.gpu_major, hwdesc.gpu_minor, hwdesc.gpu_patch);
        ESP_LOGI(TAG, "Scanout engines:   %d", hwdesc.scanout_count);
        ESP_LOGI(TAG, "RAM size:          0x%016llx", hwdesc.ram_size);
        ESP_LOGI(TAG, "IRQs implemented:  0x%08x", hwdesc.irq_impl);
        ESP_LOGI(TAG, "Required features: 0x%08x", hwdesc.required_features);
        ESP_LOGI(TAG, "Optional features: 0x%08x", hwdesc.optional_features);
        ESP_LOGI(TAG, "Coordinate bits:   %d", hwdesc.coord_bits);
    }
    return res;
}

// Do a test thingy.
esp_err_t sapphire_driver_test() {
}
