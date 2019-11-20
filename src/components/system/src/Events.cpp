#include "../include/Events.h"

#include "Wifi.h"

#include <esp_event_loop.h>
#include <esp_log.h>

namespace events {

namespace {

EventGroupHandle_t esp_events_grp;
const char *TAG = "Events";

 esp_err_t event_handler(void *ctx, system_event_t *event) {
     return wifi::handle_event(event);
 }

} // end of private namespace

void init() {
    ESP_LOGI(TAG, "init");
    esp_events_grp = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
}

void set_bit(const int bit, bool is_set) {
    if (is_set)
        xEventGroupSetBits(events::esp_events_grp, bit);
    else
        xEventGroupClearBits(events::esp_events_grp, bit);
}

} // events