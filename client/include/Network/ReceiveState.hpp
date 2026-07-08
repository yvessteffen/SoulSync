#include <Packet.hpp>
#include <vector>

struct ReceiveState
{
    PacketHeader header {};
    int headerReceived = 0;
    std::vector<uint8_t> body;
    int bodyReceived = 0;
    bool headerDone = false;
};