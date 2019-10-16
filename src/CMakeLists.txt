# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(ENV{BUILD_ESP8266} 1)

# Force using ESP IDF from libs
# IDF_PATH environment variable MUST be unset for make stage 
#unset(ENV{IDF_PATH})

# Include ESP32_IDF
#set(IDF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../lib/esp-idf)
#set(ENV{IDF_PATH} D:/repo/esp_ac_dimmer/libs/esp_idf)

#https://stackoverflow.com/questions/11390236/running-a-custom-command-in-cmake
#add_custom_command(
#OUTPUT vsphere.h
#COMMAND wsdl2h -o vsphere.h vim25/vim.wsdl
#)

#add_custom_target(vsphere_header ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vsphere.h) 

#add_dependencies(soapC vsphere_header)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello-world)
