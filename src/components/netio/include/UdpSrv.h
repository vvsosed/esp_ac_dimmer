#pragma once

namespace udp_srv {

void init();

void run();

bool sendData(const char* data, int len);

} // namespace udp_srv