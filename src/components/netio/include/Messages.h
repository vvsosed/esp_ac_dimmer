#pragma once

#include <cstdint> 
#include <array>

namespace messages {

using MsgIdType = std::uint32_t; 
using BufferType = std::array<char, 512>;

#pragma pack(push, 1)
struct DiscoveryMessage {
    static const MsgIdType ID = 0x045f0f63;
    std::uint32_t devId;
};
#pragma pack(pop)

} // namespace messages