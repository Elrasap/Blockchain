#pragma once
#include <string>
#include "types.hpp"

class Transport
{
public:
    void listen(const std::uint16_t port);
    void connect(const std::string& address);
    void receive(uint8_t* callback);
    void send(const B32 peerId, const std::uint16_t bytes);
    void close(const B32 peerId);
};
