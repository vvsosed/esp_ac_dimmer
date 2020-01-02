#include "../include/UdpSrv.h"
#include "utils.h"
#include "../include/Messages.h"

#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <sdkconfig.h>

#include <cstring>
#include <set>
#include <chrono>

#define HOST_IP_ADDR CONFIG_UDP_IO_IPV4_ADDR
#define HOST_PORT CONFIG_UDP_IO_PORT

#define DISCOVERY_MESSAGE_TIMEOUT_SEC 5 

namespace udp_srv {

namespace {
    using ClockType = std::chrono::steady_clock;
    using TimepointType = std::chrono::time_point<ClockType>;

    using DiscoveryMessage =  messages::DiscoveryMessage;
    using MsgIdType = messages::MsgIdType;
    using BufferType = messages::BufferType;

    const char *TAG = "UdpSrv";
    
    BufferType rx_buffer;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    
    int g_sock = -1;

    struct ClientInfo {
        in_addr_t addr;
        in_port_t port;
        
        ClientInfo(in_addr_t _addr, in_port_t _port)
        : addr(_addr), port(_port) {}

        bool operator< (const ClientInfo &info) const {
            return addr < info.addr || port < info.port;
        }
    };

    std::set<ClientInfo> g_clients;

   TimepointType g_discoveryTimepoint;


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
    //struct sockaddr_in destAddr;
    //destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    //destAddr.sin_family = AF_INET;
    //destAddr.sin_port = htons(HOST_PORT);
    //inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
}

bool sendDiscoveryMessage() {
    ESP_LOGI(TAG, "sendDiscoveryMessage");

    sockaddr_in destAddr = {};
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(HOST_PORT);

    decltype(DiscoveryMessage::ID) id = DiscoveryMessage::ID;
    int sz = sendto(g_sock, &id, sizeof(id), 0, reinterpret_cast<const sockaddr *>(&destAddr), sizeof(destAddr));
    if (sizeof(id) == sz) {
        DiscoveryMessage msg = {};
        msg.devId = common::getCpuId();
        sz = sendto(g_sock, &msg, sizeof(msg), 0, reinterpret_cast<const sockaddr *>(&destAddr), sizeof(destAddr));
        if (sizeof(msg) == sz)
            return true;
    }

    ESP_LOGE(TAG, "Error occured during sending: errno %d", sz);
    close_socket();
    return false;
}

void run() {
    if (open_socket() < 0)
        return;

    while ( true ) {
        struct sockaddr_in sourceAddr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(sourceAddr);
        int len = recvfrom(g_sock, rx_buffer.data(), rx_buffer.max_size(), 0, reinterpret_cast<sockaddr *>(&sourceAddr), &socklen);
        if (sizeof(MsgIdType) > len) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            close_socket();
            return;
        }
        unsigned int offset = 0;
        MsgIdType &msgId = reinterpret_cast<MsgIdType &>(rx_buffer[offset]);
        offset += sizeof(MsgIdType);
        len -= sizeof(MsgIdType);
        ESP_LOGI(TAG, "Received msgId=0x%X payload = %d bytes", msgId, len);

        switch(msgId) {
            case DiscoveryMessage::ID: {
                if (sizeof(DiscoveryMessage) > len) {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                    close_socket();
                    return;
                }
                DiscoveryMessage &msg = reinterpret_cast<DiscoveryMessage &>(rx_buffer[offset]);
                offset += sizeof(DiscoveryMessage);
                len -= sizeof(DiscoveryMessage);
                ESP_LOGI(TAG, "Received discovery message, ip=%s port=%d", toString(sourceAddr.sin_addr).c_str(), sourceAddr.sin_port);
                g_clients.emplace(sourceAddr.sin_addr.s_addr, sourceAddr.sin_port);
            }
            default:
                break;
        }

        auto currTime = ClockType::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( currTime - g_discoveryTimepoint );
        if( elapsed >= std::chrono::seconds(DISCOVERY_MESSAGE_TIMEOUT_SEC) ) {
            sendDiscoveryMessage();
            g_discoveryTimepoint = currTime;
        }
    }
}

bool sendData(const char* data, int len) {
    if (g_sock < 0) {
        return false;
    }

    sockaddr_in destAddr = {};
    destAddr.sin_family = AF_INET;
    //destAddr.sin_port = htons(HOST_PORT);
    for (const auto& info : g_clients) {
        destAddr.sin_addr.s_addr = info.addr;
        destAddr.sin_port = info.port;
        int errorCode = sendto(g_sock, data, len, 0, reinterpret_cast<const sockaddr *>(&destAddr), sizeof(destAddr));
        if (errorCode < 0) {
            ESP_LOGE(TAG, "Error occured during sending: errno %d", errorCode);
            close_socket();
            return false;
        }
    }

    return true;
}

std::string toString(const in_addr &ip4addr) {
    char addr_str[128];
    inet_ntoa_r(ip4addr, addr_str, sizeof(addr_str) - 1);
    return addr_str;
}

} // namespace udp_srv