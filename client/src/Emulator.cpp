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
    mCoreThread  thread {};
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
    impl->core->loadConfig;

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

const uint32_t* Emulator::getFramebuffer() const
{
    //return (const uint32_t*) impl->core->getPixels(impl->core);
}

unsigned Emulator::getWidth() const
{
    unsigned w, h;
    
     //TODO

    return w;
}

unsigned Emulator::getHeight() const
{
    unsigned w, h;
    
    //TODO

    return h;
}