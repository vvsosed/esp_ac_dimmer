#include "../include/utils.h"
#include "../include/FirmwareVersion.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_log.h>

namespace common {

std::uint32_t getCpuId() {
    struct {
        std::uint32_t cpuId;
        uint8_t additional[6 - sizeof(cpuId)];
    } mac;
    if(ESP_OK != esp_efuse_mac_get_default(reinterpret_cast<uint8_t *>(&mac)))
        return 0;

    mac.cpuId += mac.additional[0];
    mac.cpuId += mac.additional[1];

    return mac.cpuId;
}

void print_firmware_info() {
    const char TAG_INFO[] = "Firmware Info: ";
    ESP_LOGI(TAG_INFO, "Device ID: %d", getCpuId());
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