#include "Emulator/Emulator.hpp"

extern "C" {
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/thread.h>
#include <mgba/internal/gba/input.h>
#include <mgba/core/log.h>
#include <mgba-util/audio-buffer.h>
#include <mgba-util/audio-resampler.h>
}

#include <cstdio>
#include <iostream>
#include <SDL3/SDL.h>

#ifndef NDEBUG
#define DEBUG_LOG(x) std::cerr << x << '\n'
#else
#define DEBUG_LOG(x) ((void)0)
#endif

struct Emulator::Impl {
    mCore*       core   = nullptr;
    mCoreConfig  config {};
    uint32_t     videoBuffer[256 * 160] = {};
    mCoreOptions opts{};
    bool         ready  = false;

    mAudioBuffer    resampledBuffer {};
    mAudioResampler resampler {};
    SDL_AudioStream* audioStream = nullptr;

    Impl() {
        opts.useBios = true;
        opts.audioBuffers = 4096;
        opts.videoSync = false;
        opts.audioSync = true;
        opts.volume = 0x010;
        opts.logLevel = mLOG_WARN | mLOG_ERROR | mLOG_FATAL;
    };

    mStandardLogger _logger;
};

Emulator::Emulator() { impl = std::make_unique<Impl>(); }
Emulator::~Emulator()
{
    if (impl->audioStream)
        SDL_DestroyAudioStream(impl->audioStream);
    mAudioResamplerDeinit(&impl->resampler);
    mAudioBufferDeinit(&impl->resampledBuffer);
}

bool Emulator::initialize()
{
    impl->core = mCoreCreate(mPLATFORM_GBA);
    if (!impl->core) return false;

    impl->core->init(impl->core);
    mCoreInitConfig(impl->core, "SoulSync");
    mCoreConfigLoadDefaults(&impl->core->config, &impl->opts);
    mCoreLoadConfig(impl->core);

    mStandardLoggerInit(&impl->_logger);
    mStandardLoggerConfig(&impl->_logger, &impl->core->config);
    mLogSetDefaultLogger(&impl->_logger.d);

    impl->core->setVideoBuffer(impl->core, impl->videoBuffer, 256);
    impl->core->setAudioBufferSize(impl->core, impl->opts.audioBuffers);

    DEBUG_LOG("volume = " << impl->core->opts.volume << "\n");
    DEBUG_LOG("audioBuffers = " << impl->core->opts.audioBuffers << "\n");

    return true;
}

bool Emulator::loadRom(const std::string& path)
{
    return mCoreLoadFile(impl->core, path.c_str());
}

void Emulator::start()
{
    impl->core->reset(impl->core);
    impl->ready = true;
}

void Emulator::runFrame()
{
    impl->core->runFrame(impl->core);
}

void Emulator::setKeys(uint32_t keys)
{
    impl->core->setKeys(impl->core, keys);
}

bool Emulator::loadSave(const char* path)
{
    mCoreLoadSaveFile(impl->core, path, false);
    return true;
}

bool Emulator::initAudio(int targetSampleRate)
{
    SDL_AudioSpec spec {};
    spec.freq     = targetSampleRate;
    spec.format   = SDL_AUDIO_S16;
    spec.channels = 2;

    impl->audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr
    );
    if (!impl->audioStream) {
        DEBUG_LOG("Failed to open audio stream: " << SDL_GetError() << "\n");
        return false;
    }

    SDL_AudioSpec obtained;
    SDL_GetAudioStreamFormat(impl->audioStream, nullptr, &obtained);
    DEBUG_LOG("Audio obtained: freq=" << obtained.freq << "\n");

    mAudioBufferInit(&impl->resampledBuffer, 8192, obtained.channels);
    mAudioResamplerInit(&impl->resampler, mINTERPOLATOR_SINC);
    mAudioResamplerSetDestination(&impl->resampler, &impl->resampledBuffer, obtained.freq);

    SDL_ResumeAudioStreamDevice(impl->audioStream);
    return true;
}

void Emulator::processAudio()
{
    mAudioBuffer* src = impl->core->getAudioBuffer(impl->core);
    unsigned sampleRate = impl->core->audioSampleRate(impl->core);

    mAudioResamplerSetSource(&impl->resampler, src, sampleRate, true);
    mAudioResamplerProcess(&impl->resampler);

    int16_t data[2048];
    int remaining = (int)mAudioBufferAvailable(&impl->resampledBuffer);
    while (remaining > 0)
    {
        int thisRead = (std::min)(remaining, (int)(sizeof(data) / 4));
        int available = (int)mAudioBufferRead(&impl->resampledBuffer, data, thisRead) * 4;
        if (!available) break;
        SDL_PutAudioStreamData(impl->audioStream, data, available);
        remaining -= available;
    }
}

SDL_AudioStream* Emulator::getAudioStream() const
{
    return impl->audioStream;
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