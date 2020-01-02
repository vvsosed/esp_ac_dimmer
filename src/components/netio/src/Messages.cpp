#include "../include/Messages.h"

#include <cstring>

namespace messages {

bool pack(OutputStreamType &ostream, const DiscoveryMessage &msg) {
    return false;
}

bool unpack(DiscoveryMessage &msg, InputStreamType &istream) {
    return false;
}

} // namespace messages