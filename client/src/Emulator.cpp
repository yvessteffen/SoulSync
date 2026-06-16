#include "SoulSync/Emulator.hpp"

extern "C" {
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/thread.h>
#include <mgba/gba/core.h>
}

#include <cstdio>

struct Emulator::Impl {
    mCore*       core   = nullptr;
    mCoreConfig  config {};
    mCoreThread  thread {};
};

Emulator::Emulator() { impl = std::make_unique<Impl>(); }
Emulator::~Emulator() = default;

bool Emulator::initialize()
{
    impl->core = GBACoreCreate();
    if (!impl->core) return false;
    mCoreConfigInit(&impl->config, nullptr);
    impl->core->init(impl->core);
    mCoreLoadConfig(impl->core);
    return true;
}

bool Emulator::loadRom(const std::string& path)
{
    if (!impl->core) return false;
    return mCoreLoadFile(impl->core, path.c_str());
}

void Emulator::start()
{
    struct mCoreOptions opts = {};
    mCoreConfigLoadDefaults(&impl->config, &opts);
    impl->thread.core = impl->core;
    mCoreThreadStart(&impl->thread);
}

void Emulator::runFrame()
{
    if (!impl->core) return;
    impl->core->runFrame(impl->core);
}