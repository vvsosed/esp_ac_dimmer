#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace events {

const int WIFI_CONNECTED_BIT = BIT0;

void init();

void set_bit(const int bit, bool is_set);

} // events