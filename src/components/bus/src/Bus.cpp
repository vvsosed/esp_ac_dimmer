#include "../include/Bus.h"

#include <algorithm>
#include <freertos/task.h>

namespace bus {

constexpr int IBusClient::MESSAGE_QUEUE_LENGTH;

struct BusMsg {
    const BusAddr from;
    const BusAddr to;
    std::list<BusAddr> destination;
    std::unique_ptr<IBusCommand> msg;

    BusMsg( const BusAddr _from,
            const BusAddr _to,
            std::list<BusAddr> _destination,
            std::unique_ptr<IBusCommand>&& _msg )
    : from( _from )
    , to( _to )
    , destination( std::move( _destination ) )
    , msg( std::move( _msg ) ) {}
};

bool Bus::connect( IBusClient& client ) {
    if( !client.m_queue ) {
        return false;
    }

    LockGuardType lock( m_mutex );

    auto i = std::find_if( m_clients.begin(), m_clients.end(), [&client]( Bus::Client& other ) -> bool {
        return client.m_queue == other.queue;
    } );
    if( i != m_clients.end() ) {
        client.m_addr = i->addr;  // already connected, recovery addr
        return true;
    }

    client.m_addr = --m_addrLast;
    m_clients.emplace_front( client.m_addr, client.m_queue );

    return true;
}

void Bus::disconnect( IBusClient& client ) {
    if( !client.m_queue ) {
        return;
    }

    LockGuardType lock( m_mutex );

    std::remove_if( m_clients.begin(), m_clients.end(), [&client]( Bus::Client& other ) -> bool {
        return client.m_queue == other.queue;
    } );
    client.m_addr = BusAddrInvalid;
}

bool Bus::join( BusAddr addr, IBusClient& client ) {
    if( addr <= BusAddrInvalid || client.m_addr >= BusAddrInvalid ) {
        return false;
    }

    LockGuardType lock( m_mutex );

    auto i = std::find_if(
        m_groups.begin(), m_groups.end(), [&addr]( Bus::Group& group ) -> bool { return addr == group.addr; } );
    if( i == m_groups.end() ) {
        m_groups.emplace_front( addr );  // new group
        i = m_groups.begin();
    }

    auto j = std::find_if( i->destination.begin(), i->destination.end(), [&client]( BusAddr addr ) -> bool {
        return client.m_addr == addr;
    } );
    if( j != i->destination.end() ) {
        return true;
    }

    i->destination.emplace_front( client.m_addr );

    return true;
}

bool Bus::join( const char* addrName, IBusClient& client ) {
    if( !addrName ) {
        return false;
    }
    BusAddr addr = BusAddrInvalid;
    {
        LockGuardType lock( m_mutex );

        auto i = std::find_if( m_groups.begin(), m_groups.end(), [&addrName]( Bus::Group& group ) -> bool {
            return group.addrName == addrName;
        } );
        if( i == m_groups.end() ) {
            addr = ++m_groupLast;
            m_groups.emplace_front( addr, addrName );  // new group
        }
        else {
            addr = i->addr;
        }
    }

    return join( addr, client );
}

void Bus::leave( BusAddr addr, IBusClient& client ) {
    if( addr <= BusAddrInvalid || client.m_addr >= BusAddrInvalid ) {
        return;
    }

    LockGuardType lock( m_mutex );

    auto i = std::find_if(
        m_groups.begin(), m_groups.end(), [&addr]( Bus::Group& group ) -> bool { return addr == group.addr; } );
    if( i == m_groups.end() ) {
        return;
    }

    std::remove_if( i->destination.begin(), i->destination.end(), [&client]( BusAddr addr ) -> bool {
        return client.m_addr == addr;
    } );
}

void Bus::leave( const char* addrName, IBusClient& client ) {
    if( !addrName ) {
        return;
    }
    BusAddr addr = BusAddrInvalid;
    {
        LockGuardType lock( m_mutex );

        auto i = std::find_if( m_groups.begin(), m_groups.end(), [&addrName]( Bus::Group& group ) -> bool {
            return group.addrName == addrName;
        } );
        if( i == m_groups.end() ) {
            return;
        }
        else {
            addr = i->addr;
        }
    }

    return leave( addr, client );
}

std::list<BusAddr> Bus::destination( BusAddr addr ) {
    if( addr < 0 ) {
        return { addr };
    }
    else if( addr > 0 ) {
        LockGuardType lock( m_mutex );

        auto i = std::find_if(
            m_groups.begin(), m_groups.end(), [&addr]( Bus::Group& group ) -> bool { return addr == group.addr; } );
        if( i != m_groups.end() ) {
            return i->destination;
        }
    }

    return std::list<BusAddr>();
}

QueueHandle_t Bus::resolve( BusAddr addr ) {
    if( addr < 0 ) {
        auto i = std::find_if( m_clients.begin(), m_clients.end(), [&addr]( Bus::Client& client ) -> bool {
            return addr == client.addr;
        } );
        if( i != m_clients.end() ) {
            return i->queue;
        }
    }

    return nullptr;
}

BusAddr Bus::resolve( const char* addrName ) {
    if( !addrName ) {
        return BusAddrInvalid;
    }

    LockGuardType lock( m_mutex );

    auto i = std::find_if( m_groups.begin(), m_groups.end(), [&addrName]( Bus::Group& group ) -> bool {
        return group.addrName == addrName;
    } );
    if( i != m_groups.end() ) {
        return i->addr;
    }

    return BusAddrInvalid;
}

IBusClient::~IBusClient() {
    m_bus.disconnect( *this );
    if( m_queue ) {
        BusMsg* raw = nullptr;
        while( pdTRUE == xQueueReceive( m_queue, (void*)&raw, (portTickType)0 ) && raw ) {
            std::unique_ptr<BusMsg> busMsg( raw );
            sendToNextClient( std::move( busMsg ) );
            raw = nullptr;
        }
        vQueueDelete( m_queue );
        m_queue = nullptr;
    }
}

bool IBusClient::initialize() {
    if( !m_queue ) {
        m_queue = xQueueCreate( MESSAGE_QUEUE_LENGTH, sizeof( BusMsg* ) );
        if( m_queue ) {
            m_bus.connect( *this );
        }
    }
    return isConnectedToBus();
}

bool IBusClient::isConnectedToBus() {
    return m_queue && ( BusAddrInvalid != m_addr );
}

void IBusClient::send( const char* to, std::unique_ptr<IBusCommand> msg ) const {
    if( !to || !msg ) {
        return;
    }

    send( m_bus.resolve( to ), std::move( msg ) );
}

void IBusClient::send( BusAddr to, std::unique_ptr<IBusCommand> msg ) const {
    if( to == BusAddrInvalid || !msg ) {
        return;
    }

    std::unique_ptr<BusMsg> busMsg(
        std::make_unique<BusMsg>( m_addr, to, std::move( m_bus.destination( to ) ), std::move( msg ) ) );
    sendToNextClient( std::move( busMsg ) );
}

void IBusClient::tryReceive( TickType_t waitTime ) {
    if( m_queue ) {
        BusMsg* raw = nullptr;
        if( pdTRUE == xQueueReceive( m_queue, (void*)&raw, (portTickType)waitTime ) && raw ) {
            std::unique_ptr<BusMsg> busMsg( raw );
            receive( busMsg->from, busMsg->to, busMsg->msg );
            sendToNextClient( std::move( busMsg ) );
            return;
        }
    }
    vTaskDelay( waitTime );
}

void IBusClient::sendToNextClient( std::unique_ptr<BusMsg> busMsg ) const {
    if( !busMsg || busMsg->destination.empty() ) {
        return;
    }

    LockGuardType lock( m_bus.m_mutex );

    QueueHandle_t dest = nullptr;
    do {  // try resolve valid client queue
        dest = m_bus.resolve( busMsg->destination.back() );
        busMsg->destination.pop_back();
    } while( !busMsg->destination.empty() && !dest );
    if( !dest ) {
        return;
    }

    auto raw = busMsg.get();
    if( pdTRUE == xQueueSend( dest, (void*)&raw, (portTickType)0 ) ) {
        busMsg.release();
    }
}

}  // namespace bus
