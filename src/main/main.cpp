/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "utils.h"

#include "onewire.h"

TickType_t  msecToSysTick( std::uint32_t xTimeInMs) {
    return (TickType_t)xTimeInMs / portTICK_PERIOD_MS ;
} 

extern "C" void app_main()
{
    common::print_firmware_info();

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    auto readOWPinClbk = []() -> bool {
        return false;
    };

    auto setPinModeClbk = []( const onewire::PinMode pinMode ) {

    };

    auto setPinValueClbk = []( const bool pinValue ) {

    };

    auto setIntrModeClbk = [](const onewire::IntMode intrMode) {
        if ( onewire::INTR_ON == intrMode ) {
            //portENABLE_INTERRUPTS();
        }
        else {
            //portDISABLE_INTERRUPTS();
        }
    };

    auto delayMsecClbk = []( const uint32_t delayMsec ) {
        vTaskDelay(msecToSysTick (delayMsec) );
    };

    onewire::OneWire ow_inst( setPinModeClbk,
                              readOWPinClbk, 
                              setPinValueClbk, 
                              delayMsecClbk, 
                              setIntrModeClbk );
    
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
