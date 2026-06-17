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
    const uint32_t* getFramebuffer() const;
    unsigned getWidth() const;
    unsigned getHeight() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};