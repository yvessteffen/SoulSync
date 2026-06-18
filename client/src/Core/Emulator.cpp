#include "Emulator.hpp"

extern "C" {
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/thread.h>
}

#include <cstdio>
#include <iostream>

struct Emulator::Impl {
    mCore*       core   = nullptr;
    mCoreConfig  config {};
    uint32_t     videoBuffer[256 * 160] = {};
};

Emulator::Emulator() { impl = std::make_unique<Impl>(); }
Emulator::~Emulator() = default;

bool Emulator::initialize()
{
    impl->core = mCoreCreate(mPLATFORM_GBA);
    if (!impl->core) return false;

    impl->core->init(impl->core);

    printf("core        = %p\n", (void*)impl->core);
    printf("init        = %p\n", (void*)impl->core->init);
    printf("loadConfig  = %p\n", (void*)impl->core->loadConfig);
    printf("loadROM     = %p\n", (void*)impl->core->loadROM);
    printf("unloadROM   = %p\n", (void*)impl->core->unloadROM);

    mCoreInitConfig(impl->core, nullptr);
    //impl->core->loadConfig(impl->core, &impl->config);
    impl->core->setVideoBuffer(impl->core, impl->videoBuffer, 256);

    return true;
}

bool Emulator::loadRom(const std::string& path)
{
    return mCoreLoadFile(impl->core, path.c_str());
}

void Emulator::start()
{
    impl->core->reset(impl->core);
}

void Emulator::runFrame()
{
    impl->core->runFrame(impl->core);
}

Framebuffer Emulator::getFramebuffer() const
{
    const void* buffer = nullptr;
    size_t stride = 0;

    impl->core->getPixels(impl->core, &buffer, &stride);

    unsigned w = 0, h = 0;
    impl->core->currentVideoSize(impl->core, &w, &h);

    return { buffer, stride, w, h };
}
