#pragma once

#include <string>
#include <memory>

struct Framebuffer
{
    const void* pixels;
    size_t stride;
};

class Emulator {
public:
    Emulator();
    ~Emulator();

    bool initialize();
    bool loadRom(const std::string& path);
    void start();
    void runFrame();

    unsigned getVideoWidth() const;
    unsigned getVideoHeight() const;

    Framebuffer getFramebuffer() const;


private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};