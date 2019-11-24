#pragma once

#include <memory>

namespace onewire {

class TemperatureSensorBase {
    public:
    using UPtr = std::unique_ptr<TemperatureSensorBase>;

    static UPtr create();
};

} // namespace onewire