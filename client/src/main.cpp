#include <iostream>
#include "Emulator.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono;
constexpr auto FRAME_DURATION = duration<double>(1.0 / 59.7275);

int main()
{
    std::cout << "SoulSync Client\n";

    Emulator emulator;

    if (!emulator.initialize()) {
        std::cout << "init failed\n";
        return 1;
    }
    std::cout << "Emulator initialized\n";

    const char* path = "C:\\Users\\yvess\\Desktop\\Private Projects\\SoulSync\\build\\client\\Release\\rom.gba";    
    if (!emulator.loadRom(path)) {
        std::cerr << "Failed to load ROM\n";
        return 1;
    }
    std::cout << "Rom loaded\n";

    emulator.start();
    std::cout << "Emulator started\n";

    while (true) {
        auto start = steady_clock::now();
        emulator.runFrame();
        auto elapsed = steady_clock::now() - start;
        std::this_thread::sleep_for(FRAME_DURATION - elapsed);
    }

    return 0;
}