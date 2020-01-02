#include <esp_wifi.h>
#include <string.h>

void DefaultInit(wifi_init_config_t *cfg) {
    wifi_init_config_t c = WIFI_INIT_CONFIG_DEFAULT();
    memcpy(cfg, &c, sizeof(wifi_init_config_t));
}