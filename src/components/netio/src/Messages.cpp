#include "../include/Messages.h"

#include "InputStream.h"
#include "OutputStream.h"


#include <cstring>

namespace messages {

namespace {

    Callbacks g_callbacks;

} // private namespace

Callbacks getCallbacs() {
    return g_callbacks;
}

void setCallbacks(Callbacks &&callbacs) {
    g_callbacks = std::move(callbacs);
}

void setCallback(std::function<void(Discovery &)> callback) {
    g_callbacks.onDiscovery = callback;
}

void setCallback(std::function<void(Temperature &)> callback) {
    g_callbacks.onTemperature = callback;
}

bool parse(InputStreamType& stream) {
    return false;
}

} // namespace messages