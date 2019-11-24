#pragma once

#include <memory>

#include "../include/onewire.h"

namespace onewire {

inline float celsiusToFahrenheit( const float& celsius ) { return celsius * 1.8 + 32.0; }

class TemperatureSensor {
public:
    using UPtr = std::unique_ptr<TemperatureSensor>;

    static UPtr create( OneWire& oneWire, const RegisterNumber& regNum );

    TemperatureSensor( OneWire& oneWire, const RegisterNumber& regNum )
    : m_oneWire(oneWire), m_regNum(regNum) {}

    virtual ~TemperatureSensor() = default;

    virtual bool measureTemperature() = 0;

    virtual float getCelsius() const = 0;

    inline float getFahrenheit() const { return celsiusToFahrenheit(getCelsius()); }

protected:
    OneWire& m_oneWire;
    RegisterNumber m_regNum;
};

class DS18B20_TemperatureSensor : public TemperatureSensor {
public:
    using TemperatureSensor::TemperatureSensor;

    bool measureTemperature() override;

    float getCelsius() const override;

protected:
    std::uint8_t m_data[9];
};

class DS18S20_DS18S20_TemperatureSensor : public DS18B20_TemperatureSensor {
public:
    using DS18B20_TemperatureSensor::DS18B20_TemperatureSensor;

    float getCelsius() const override;
};

} // namespace onewire