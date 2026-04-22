#include "../../include/protocol/RdtEndpoint.hpp"

RdtEndpoint::RdtEndpoint(UdpSocket &socket, TimerManager &timer_manager,
                         const Config &config)
    : socket(socket), timer_manager(timer_manager), config(config) {}
