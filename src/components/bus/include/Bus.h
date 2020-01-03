#pragma once

#include "BusAddr.h"
#include "BusCommand.h"

#include <stdint.h>
#include <list>
#include <string>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace bus {

#define USE_FREE_RTOS_MUTEX 1
#ifdef USE_FREE_RTOS_MUTEX
#include "freertos/semphr.h"

class FreeRtosMutex {
public:
    FreeRtosMutex()
    : m_semaphore( xSemaphoreCreateMutex() ) {}

    ~FreeRtosMutex() {
        vSemaphoreDelete( m_semaphore );
        m_semaphore = nullptr;
    }

    FreeRtosMutex( const FreeRtosMutex& other ) = delete;
    FreeRtosMutex( const FreeRtosMutex&& other ) = delete;
    FreeRtosMutex& operator=( const FreeRtosMutex& other ) = delete;

    inline bool lock() {
        return ( pdPASS == xSemaphoreTake( m_semaphore, portMAX_DELAY ) );
    }

    inline bool release() {
        return ( pdPASS == xSemaphoreGive( m_semaphore ) );
    }

private:
    SemaphoreHandle_t m_semaphore;
};

class FreeRtosMutexLock {
public:
    explicit FreeRtosMutexLock( FreeRtosMutex& lock )
    : m_lock( lock ) {
        m_isLocked = m_lock.lock();
    }

    ~FreeRtosMutexLock() {
        release();
    }

    FreeRtosMutexLock( const FreeRtosMutexLock& other ) = delete;
    FreeRtosMutexLock( const FreeRtosMutexLock&& other ) = delete;
    FreeRtosMutexLock& operator=( const FreeRtosMutexLock& other ) = delete;

    inline bool isLocked() const {
        return m_isLocked;
    }

    inline bool release() {
        if( m_isLocked ) {
            m_isLocked = !m_lock.release();
        }

        return !m_isLocked;
    }

private:
    FreeRtosMutex &m_lock;
    bool m_isLocked = false;
};

using MutexType = FreeRtosMutex;
using LockGuardType = FreeRtosMutexLock;

#else
#include <mutex>
using MutexType = std::mutex;
#endif

struct BusMsg;
class IBusClient;

class Bus {
    friend class IBusClient; // use Bus mutex
public:
    bool connect( IBusClient& client );
    void disconnect( IBusClient& client );

    bool join( const char* addrName, IBusClient& client );
    void leave( const char* addrName, IBusClient& client );

    std::list<BusAddr> destination( BusAddr addr );
    QueueHandle_t resolve( BusAddr addr );
    BusAddr resolve( const char* addrName );

protected:
    bool join( BusAddr addr, IBusClient& client );
    void leave( BusAddr addr, IBusClient& client );

private:
    MutexType m_mutex;
    BusAddr m_addrLast = BusAddrInvalid;
    BusAddr m_groupLast = BusAddrInvalid;

    struct Client {
        BusAddr addr;
        QueueHandle_t queue;

        Client( BusAddr _addr, QueueHandle_t _queue )
        : addr( _addr )
        , queue( _queue ) {}
    };
    std::list<Client> m_clients;

    struct Group {
        BusAddr addr;
        std::string addrName;
        std::list<BusAddr> destination;

        Group( BusAddr _addr, const char* _addrName = nullptr )
        : addr( _addr )
        , addrName( _addrName ) {}
    };
    std::list<Group> m_groups;
};

class IBusClient {
    friend class Bus;

public:
    static constexpr int MESSAGE_QUEUE_LENGTH = 32;

    IBusClient(Bus &bus)
    : m_bus(bus) {}

    virtual ~IBusClient();

    bool initialize();
    bool isConnectedToBus();

    void send( BusAddr to, std::unique_ptr<IBusCommand> msg ) const;
    void send( const char* to, std::unique_ptr<IBusCommand> msg ) const;

    BusAddr addr() const {
        return m_addr;
    }

protected:
    void tryReceive( TickType_t waitTime );
    
    virtual void receive( const BusAddr from, const BusAddr to, const std::unique_ptr<IBusCommand>& msg ) = 0;

private:
    void sendToNextClient( std::unique_ptr<BusMsg> busMsg ) const;

private:
    Bus &m_bus;
    BusAddr m_addr = BusAddrInvalid;
    QueueHandle_t m_queue = nullptr;
};

struct EventsHandler {
    BusAddr m_addr;
    std::string m_handlerName;
};

template <typename Visitor>
class BusMessageHandler : public IBusClient, public Visitor {
protected:
    void receive( const BusAddr from, const BusAddr to, const std::unique_ptr<IBusCommand>& msg ) override {
        msg->accept(from, to, *this);
    }
};

}  // namespace bus
