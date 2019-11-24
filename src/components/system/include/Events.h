#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace events {

const int WIFI_CONNECTED_BIT = BIT0;

void init();

void set_bit( const int bit );

void clear_bit( const int bit );

void wait_for( const int bits, const TickType_t& waitTicks = portMAX_DELAY );

int get_bit( const int bits );

inline void wait_connection( const TickType_t& waitTicks = portMAX_DELAY ) {
    wait_for(WIFI_CONNECTED_BIT, waitTicks);
}

} // events