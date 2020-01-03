#pragma once

#include "BusAddr.h"
#include "sdkconfig.h"

namespace bus {
namespace command {

struct ExampleCmd1;
struct ExampleCmd2;

}  // namespace command

struct IBusCommandVisitor {
    virtual ~IBusCommandVisitor() = default;

    virtual void visit( const BusAddr from, const BusAddr to, command::ExampleCmd1& command ) {}
    virtual void visit( const BusAddr from, const BusAddr to, command::ExampleCmd2& command ) {}
};

struct IBusCommand {
    virtual ~IBusCommand() = default;

    virtual void accept( const BusAddr from, const BusAddr to, IBusCommandVisitor& client ) = 0;
};

}  // namespace bus
