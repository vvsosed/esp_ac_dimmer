#include "../include/utils.h"
#include "../include/FirmwareVersion.h"

#include "esp_log.h"

namespace common {

void print_firmware_info() {
    ESP_LOGW("Firmware Info: ", "Hardware arch: %s", HARDWARE_ARCH);
    ESP_LOGW("Firmware Info: ", "Firmware version: %s", FIRMWARE_VERSION);
    ESP_LOGW("Firmware Info: ", "Kernel version: %s", KERNEL_VERSION);
    ESP_LOGW("Firmware Info: ", "Build time: %s", BUILD_TIME);
    ESP_LOGW("Firmware Info: ", "Builder: %s", BUILD_BUILDER);
    ESP_LOGW("Firmware Info: ", "Git branch: %s", BUILD_BRANCH);
    ESP_LOGW("Firmware Info: ", "Git commit: %s\n", BUILD_COMMIT);
}

} // end of namespae common