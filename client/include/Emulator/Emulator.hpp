#pragma once

#include <string>
#include <memory>
#include <mutex>

struct Framebuffer{
    const void* pixels;
    size_t stride;
    unsigned width;
    unsigned height;
};

struct SDL_AudioStream;

class Emulator {
public:
    Emulator();
    ~Emulator();
    bool initialize();
    bool loadRom(const std::string& path);
    bool loadSave(const char* path);
    void start();
    void runFrame();
    void setKeys(uint32_t keys);
    bool initAudio(int targetSampleRate);
    void processAudio();
    SDL_AudioStream* getAudioStream() const;
    unsigned getVideoWidth() const;
    unsigned getVideoHeight() const;
    Framebuffer getFramebuffer() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};