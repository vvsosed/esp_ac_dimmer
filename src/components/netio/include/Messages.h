#pragma once

#include "InputStream.h"
#include "OutputStream.h"

#include <cstdint> 
#include <array>
#include <iostream>
#include <functional>


namespace messages {

using MsgIdType = std::uint32_t; 
using BufferType = std::array<char, 512>;
using OutputStreamType = streams::OutputBase;
using InputStreamType = streams::InputBase;

struct Discovery {
    static const MsgIdType ID = 0x045f0f63;
    
    std::uint32_t devId;

    inline bool write( OutputStreamType& stream ) const {
        return stream.write( devId );
    }

    inline bool read( InputStreamType& stream ) {
        return stream.read( devId );
    }
};

struct Temperature {
    static const MsgIdType ID = 0x3eee0c83;

    std::uint64_t sensorId;
    float value;

    inline bool write( OutputStreamType& stream ) const {
        return stream.write( sensorId ) && stream.write( value );
    }

    inline bool read( InputStreamType& stream ) {
        return stream.read( sensorId ) && stream.read( value );
    }
};

template <typename Msg, typename Buffer = BufferType>
inline auto create(Buffer &buffer, const Msg &msg) {
    streams::ArrayOutputStream ostream(buffer.data(), buffer.max_size());
    const MsgIdType id = msg.ID;
    bool isSuccess = ostream.write(id) && ostream.write(msg);
    return isSuccess ? ostream.dataSize() : 0;
}

struct Callbacks {
    std::function<void(Discovery &)> onDiscovery;
    std::function<void(Temperature &)> onTemperature;
};


Callbacks getCallbacs();

void setCallbacks(Callbacks &&callbacs);

void setCallback(std::function<void(Discovery &)> callback);

void setCallback(std::function<void(Temperature &)> callback);

bool parse(InputStreamType& stream);

} // namespace messages