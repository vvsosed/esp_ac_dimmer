set(COMPONENT_SRCS "src/Wifi.cpp" "src/Wifi.c" "src/UdpSrv.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_PRIV_REQUIRES "tcpip_adapter" "system")

register_component()