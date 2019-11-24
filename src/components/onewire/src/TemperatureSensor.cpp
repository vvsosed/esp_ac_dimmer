#include "../include/TemperatureSensor.h"
#include "utils.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
namespace onewire {

namespace {
    const char* TAG = "OWTempSens";
} // private namespace

TemperatureSensor::UPtr TemperatureSensor::create( OneWire& oneWire, const RegisterNumber& regNum ) {
    switch (regNum.family_code) {
    case 0x10:
        ESP_LOGI(TAG, "Chip = DS18S20");  // or old DS1820
        //type_s = 1;
        return std::make_unique<DS18S20_DS18S20_TemperatureSensor>(oneWire, regNum);
    case 0x28:
        ESP_LOGI(TAG, "Chip = DS18B20");
        //type_s = 0;
        return std::make_unique<DS18B20_TemperatureSensor>(oneWire, regNum);
    case 0x22:
        ESP_LOGI(TAG, "Chip = DS1822");
        //type_s = 0;
        return std::make_unique<DS18B20_TemperatureSensor>(oneWire, regNum);
    default:
        ESP_LOGI(TAG, "Device is not a DS18x20 family device.");
        return nullptr;
    }
}

bool DS18B20_TemperatureSensor::measureTemperature() {
    if ( !m_oneWire.reset() ) {
            ESP_LOGW(__FUNCTION__, "1 Presence is absent!");
            return false;
        }

    m_oneWire.select(m_regNum.romData);
    m_oneWire.write(0x44, 1); // start conversion, with parasite power on at the end
    vTaskDelay( common::msecToSysTick(10) );

    // we might do a ds.depower() here, but the reset will take care of it.
        
    if ( !m_oneWire.reset() ) {
        ESP_LOGW(TAG, "2 Presence is absent!");
        return false;
    }

    m_oneWire.select(m_regNum.romData);   
    m_oneWire.write(0xBE); // Read Scratchpad
    for ( int i = 0; i < sizeof(m_data); i++) 
        m_data[i] = m_oneWire.read();
    
    if (crc8(m_data, 8) != m_data[8]) {
        ESP_LOGI(TAG, "Bad Data CRC: (calc=0x%X) != (receiv=0x%X)\n", crc8(m_data, 8), m_data[8]);
        return false;
    }

    return true;
}

float DS18B20_TemperatureSensor::getCelsius() const {
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    std::int16_t raw = (m_data[1] << 8) | m_data[0];
    std::uint8_t cfg = (m_data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time

    return float(raw) / 16.0;
}

float DS18S20_DS18S20_TemperatureSensor::getCelsius() const {
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    std::int16_t raw = (m_data[1] << 8) | m_data[0];
    raw = raw << 3; // 9 bit resolution default
    if (m_data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - m_data[6];
    }

    return float(raw) / 16.0;
}

} // namespace onewire