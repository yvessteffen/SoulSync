#include <Video/Decoder.hpp>
#include <Network/ReceiveState.hpp>
#include <UI/ManagedWindow.hpp>

struct StreamState
{
    ManagedWindow window;

    Decoder decoder;
    ReceiveState recv;

    std::vector<uint8_t> pixels;
    int w = 0, h = 0;

    bool initialized = false;
};