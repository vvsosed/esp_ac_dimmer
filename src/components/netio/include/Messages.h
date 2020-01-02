#pragma once

#include <cstdint> 
#include <array>
#include <iostream>
#include <streambuf>

namespace messages {

using MsgIdType = std::uint32_t; 
using BufferType = std::array<char, 512>;
using OutputStreamType = BufferType;
using InputStreamType = BufferType;

#pragma pack(push, 1)
struct DiscoveryMessage {
    static const MsgIdType ID = 0x045f0f63;
    std::uint32_t devId;
};
#pragma pack(pop)

bool pack(OutputStreamType &ostream, const DiscoveryMessage &msg);

bool unpack(DiscoveryMessage &msg, InputStreamType &istream);

} // namespace messages