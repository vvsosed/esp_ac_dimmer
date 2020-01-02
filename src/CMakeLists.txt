# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(ENV{BUILD_ESP8266} 1)

# Force using ESP IDF from libs
# IDF_PATH environment variable MUST be unset for make stage 
#unset(ENV{IDF_PATH})

# Include ESP32_IDF
#set(IDF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../lib/esp-idf)
#set(ENV{IDF_PATH} D:/repo/esp_ac_dimmer/libs/esp_idf)

add_custom_command(
OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/components/common/include/FirmwareVersion.h
COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/version.py --major 0 --minor 0 --build 1
)
add_custom_target(firmware_version ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/components/common/include/FirmwareVersion.h) 
#add_dependencies(hello-world firmware_version)

add_custom_target(monitor
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND python $ENV{IDF_PATH}/tools/idf.py monitor
    USES_TERMINAL
    )
    
#add_custom_target(idf_flash
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    COMMAND python $ENV{IDF_PATH}/tools/idf.py -p COM5 -b 115200 flash
#    USES_TERMINAL
#	)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello-world)
