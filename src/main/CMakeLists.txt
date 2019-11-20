set(COMPONENT_SRCS "main.cpp")
set(COMPONENT_REQUIRES "common" "onewire")
set(COMPONENT_PRIV_REQUIRES "tcpip_adapter" "netio" "system" "nvs_flash") 

register_component()

