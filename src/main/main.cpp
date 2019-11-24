#include <stdio.h>
#include <future>
//#include <cstring>
//#include <cstdio>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
//#include <esp_spi_flash.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>

#include <driver/gpio.h>

#include "utils.h"
#include "onewire.h"
#include "Events.h"
#include "Wifi.h"
#include "UdpSrv.h"

namespace {

static const char *TAG = "main";

auto InitialPinMask = GPIO_Pin_5;// | GPIO_Pin_4;
auto LedPin = GPIO_NUM_5;
auto OWPin = GPIO_NUM_4;

inline TickType_t  msecToSysTick( std::uint32_t xTimeInMs) {
    return (TickType_t)xTimeInMs / portTICK_PERIOD_MS ;
} 

void gpio_task_example(void *arg)
{
    for (int cnt = 1; true; cnt++) {
        //ESP_LOGI(TAG, "cnt: %d", cnt);
        vTaskDelay( msecToSysTick(1000) );
        gpio_set_level(LedPin, cnt % 2);
    }
}

void udp_srv_task(void *arg)
{
    udp_srv::run();
    vTaskDelete(NULL);
}

void IRAM_ATTR setIntrModeClbk (const onewire::IntMode intrMode) {};

bool IRAM_ATTR readOWPinClbk() {
    return  gpio_get_level(OWPin);
};


void IRAM_ATTR setPinModeClbk( const onewire::PinMode pinMode ) {
    gpio_set_direction(OWPin, onewire::PinMode::OUTPUT == pinMode ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_INPUT);
};

void IRAM_ATTR setPinValueClbk( const bool pinValue ) {
    gpio_set_level(OWPin, pinValue);
};

} // end of private namespace

extern "C" void app_main()
{
    common::print_firmware_info();
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    events::init();

    gpio_config_t io_conf;    
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = InitialPinMask;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    xTaskCreate(gpio_task_example, "gpio_task_example", 256, NULL, 10, NULL);

    wifi::init_station();
    events::wait_connection();
    udp_srv::init();
    xTaskCreate(udp_srv_task, "udp_srv_task", 2048, NULL, 10, NULL);

    onewire::OneWire ow_inst( setPinModeClbk,
                              readOWPinClbk, 
                              setPinValueClbk, 
                              ets_delay_us, 
                              setIntrModeClbk );

    
    ESP_LOGI(__FUNCTION__, "Start pooling OneWire devices...\n\n\n");
    
    for (int indx = 1; true; indx++) {
        vTaskDelay( msecToSysTick(500) );
        
        onewire::RegisterNumber regNum;
        std::uint8_t *addr = regNum.romData;
        
        if ( !ow_inst.search(regNum.romData) ) {
            ESP_LOGI(__FUNCTION__, "No more addresses.\n\n\n");
            ow_inst.reset_search();
            continue;
        }
        
        if ( !regNum.isCrcValid() ) {
            ESP_LOGE(__FUNCTION__, "CRC is not valid!");
            continue;
        }
        
        char idStr[64];
        snprintf(idStr, sizeof(idStr), "0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X",
                 addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6]);
        ESP_LOGI(__FUNCTION__, "ADDR =[%s]", idStr);

        std::uint8_t  type_s = 0;
        // the first ROM byte indicates which chip
        switch (addr[0]) {
        case 0x10:
            ESP_LOGI(__FUNCTION__, "Chip = DS18S20");  // or old DS1820
            type_s = 1;
            break;
        case 0x28:
            ESP_LOGI(__FUNCTION__, "Chip = DS18B20");
            type_s = 0;
            break;
        case 0x22:
            ESP_LOGI(__FUNCTION__, "Chip = DS1822");
            type_s = 0;
            break;
        default:
            ESP_LOGI(__FUNCTION__, "Device is not a DS18x20 family device.");
            continue;
        }
                
        if ( !ow_inst.reset() ) {
            ESP_LOGW(__FUNCTION__, "1 Presence is absent!");
            continue;
        }

        ow_inst.select(addr);
        ow_inst.write(0x44, 1); // start conversion, with parasite power on at the end
        vTaskDelay( msecToSysTick(10) );
        // we might do a ds.depower() here, but the reset will take care of it.
        
        if ( !ow_inst.reset() ) {
            ESP_LOGW(__FUNCTION__, "2 Presence is absent!");
            continue;
        }

        ow_inst.select(addr);   
        ow_inst.write(0xBE);         // Read Scratchpad
        std::uint8_t data[9]; // we need 9 bytes
        for ( int i = 0; i < sizeof(data); i++) 
            data[i] = ow_inst.read();
        
        if (onewire::crc8(data, 8) != data[8]) {
            ESP_LOGI(__FUNCTION__, "Bad Data CRC: (calc=0x%X) != (receiv=0x%X)\n", onewire::crc8(data, 8), data[8]);
            continue;
        }
               
        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        std::int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
            }
        } else {
            std::uint8_t cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
            //// default is 12 bit resolution, 750 ms conversion time
        }

        auto celsius = float(raw) / 16.0;
        auto fahrenheit = celsius * 1.8 + 32.0;
        char buf[128];
        auto respLen = snprintf(
            buf, 
            sizeof(buf), 
            "ID[%s] Temp: Cels=%0.2f Fahr=%0.2f\n", 
            idStr, celsius, fahrenheit);
        udp_srv::sendData(buf, respLen);
        ESP_LOGI(__FUNCTION__, "%s", buf);
    }

    ESP_LOGI(TAG, "Restarting now...");
    fflush(stdout);
    esp_restart();
}
