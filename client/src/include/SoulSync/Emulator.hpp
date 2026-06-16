#pragma once

#include <string>
#include <memory>

class Emulator {
public:
    Emulator();
    ~Emulator();
    bool initialize();
    bool loadRom(const std::string& path);
    void start();
    void runFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};