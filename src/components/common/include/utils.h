#pragma once

#include <cstdint>
#include <portmacro.h>

namespace common {

std::uint32_t getCpuId();

void print_firmware_info();

inline TickType_t msecToSysTick( std::uint32_t xTimeInMs) {
    return (TickType_t)xTimeInMs / portTICK_PERIOD_MS;
} 

}  // end of namespae common