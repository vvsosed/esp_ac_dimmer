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
#include "TemperatureSensor.h"
#include "Events.h"
#include "Wifi.h"
#include "UdpSrv.h"

namespace {

static const char *TAG = "main";

auto InitialPinMask = GPIO_Pin_5;// | GPIO_Pin_4;
auto LedPin = GPIO_NUM_5;
auto OWPin = GPIO_NUM_4;

void gpio_task_example(void *arg)
{
    for (int cnt = 1; true; cnt++) {
        //ESP_LOGI(TAG, "cnt: %d", cnt);
        vTaskDelay( common::msecToSysTick(1000) );
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

#pragma pack(push, 1) 
struct TempMsg {
    std::uint64_t id;
    float temp_cels;
};
#pragma pack(pop)

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
        vTaskDelay( common::msecToSysTick(500) );
        
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
        //memset(idStr, 0, sizeof(idStr));
        snprintf(
            idStr, 
            sizeof(idStr), 
            "0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6]);
        ESP_LOGI(__FUNCTION__, "ADDR =[%s]", idStr);

        auto tempSens = onewire::TemperatureSensor::create(ow_inst, regNum);
        if ( !tempSens || !tempSens->measureTemperature() ) {
            ESP_LOGI(TAG, "Bad sensor %s", idStr);
            continue;
        }

        auto celsius = tempSens->getCelsius();
        auto fahrenheit = onewire::celsiusToFahrenheit(celsius);
        char buf[128];
        //memset(buf, 0, sizeof(buf));
        auto respLen = snprintf(
            buf, 
            sizeof(buf), 
            "ID[%s] Temp: Cels=%0.2f Fahr=%0.2f\n", 
            idStr, celsius, fahrenheit);
        TempMsg msg = {};
        msg.id = regNum.val64;
        msg.temp_cels = celsius;
        udp_srv::sendData(reinterpret_cast<char*>(&msg), sizeof(msg));
        ESP_LOGI(__FUNCTION__, "%s", buf);
    }

    ESP_LOGI(TAG, "Restarting now...");
    fflush(stdout);
    esp_restart();
}
