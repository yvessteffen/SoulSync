#pragma once

#include <string>
#include <memory>

struct Framebuffer{
    const void* pixels;
    size_t stride;
    unsigned width;
    unsigned height;
};

class Emulator {
public:
    Emulator();
    ~Emulator();

    bool initialize();
    bool loadRom(const std::string& path);
    void start();
    void runFrame();
    
    Framebuffer getFramebuffer() const;


private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};