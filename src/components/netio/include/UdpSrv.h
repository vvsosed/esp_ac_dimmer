#pragma once

#include <string>

#include <lwip/inet.h>

namespace udp_srv {

void init();

void run();

bool sendData(const char* data, int len);

std::string toString(const in_addr &ip4addr); 

} // namespace udp_srv