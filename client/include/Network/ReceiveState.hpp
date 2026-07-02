#pragma once
#include <Packet.hpp>
#include <vector>

struct ReceiveState
{
    PacketHeader header{};
    int headerReceived = 0;
    bool headerDone = false;

    std::vector<uint8_t> body;
    int bodyReceived = 0;
};