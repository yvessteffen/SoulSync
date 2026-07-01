#pragma once
#include <cstdint>
#include <cstring>

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t type;
    char senderId[32];
    uint32_t size;
};
#pragma pack(pop)