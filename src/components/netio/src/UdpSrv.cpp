#include "../include/UdpSrv.h"

#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <sdkconfig.h>

#include <cstring>

#define HOST_IP_ADDR CONFIG_UDP_IO_IPV4_ADDR
#define HOST_PORT CONFIG_UDP_IO_PORT

namespace udp_srv {

namespace {
    const char *TAG = "UdpSrv";
    
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in destAddr;

    int g_sock = -1;

    int open_socket() {
        g_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if ( 0 > g_sock)
            ESP_LOGE(TAG, "Unable to create socket: errno %d", g_sock);
        else
            ESP_LOGI(TAG, "Socket created, sock=%d", g_sock);
        return g_sock;
    }

    void close_socket() {
        if (g_sock < 0)
            return;
        
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(g_sock, 0);
        close(g_sock);
        g_sock = -1;
    }
} // private  namespace

void init() {
    
    //destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(HOST_PORT);
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
}

void run() {
    while( true ) {
        if (open_socket() < 0)
            break;

        while ( true ) {
            struct sockaddr_in sourceAddr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(g_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                close_socket();
                break;
            }
            
            rx_buffer[len] = '\0'; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received: %s", len > 0 ? rx_buffer : "Empty UDP packet");
        }
    }
}

bool sendData(const char* data, int len) {
    if (g_sock < 0) {
        return false;
    }

    int errorCode = sendto(g_sock, data, len, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (errorCode < 0) {
        ESP_LOGE(TAG, "Error occured during sending: errno %d", errorCode);
        close_socket();
        return false;
    }

    return true;
}

} // namespace udp_srv