#include "../include/utils.h"
#include "../include/FirmwareVersion.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_log.h>

namespace common {

void print_firmware_info() {
    const char TAG_INFO[] = "Firmware Info: ";
    ESP_LOGI(TAG_INFO, "Hardware arch: %s", HARDWARE_ARCH);
    ESP_LOGI(TAG_INFO, "Firmware version: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG_INFO, "Kernel version: %s", KERNEL_VERSION);
    ESP_LOGI(TAG_INFO, "Build time: %s", BUILD_TIME);
    ESP_LOGI(TAG_INFO, "Builder: %s", BUILD_BUILDER);
    ESP_LOGI(TAG_INFO, "Git branch: %s", BUILD_BRANCH);
    ESP_LOGI(TAG_INFO, "Git commit: %s", BUILD_COMMIT);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG_INFO, "This is ESP8266 chip with %d CPU cores", chip_info.cores);
    ESP_LOGI(TAG_INFO, "Silicon revision %d", chip_info.revision);
    auto flashSizeMb = spi_flash_get_chip_size() / (1024 * 1024);
    auto flashType = (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external";
    ESP_LOGI(TAG_INFO, "%dMB %s flash", flashSizeMb, flashType);
}

} // end of namespae common