#pragma once

#include "esp_event_loop.h"

namespace wifi {

void init_acess_point();

void init_station();

esp_err_t handle_event(system_event_t *event);

} // namespace wifi