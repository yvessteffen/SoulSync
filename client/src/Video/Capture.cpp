#include "Video/Capture.hpp"

#include <cstring>
#include <SDL3/SDL.h>

Capture::Capture(int width, int height)
    : mWidth(width), mHeight(height)
{
}

Frame Capture::capture(const uint32_t* framebuffer, uint64_t frameNumber)
{
    Frame frame;
    frame.frameNumber = frameNumber;
    frame.timestamp   = SDL_GetTicks();
    frame.width       = mWidth;
    frame.height      = mHeight;
    frame.pixels.resize(mWidth * mHeight * 4);
    std::memcpy(frame.pixels.data(), framebuffer, frame.pixels.size());
    return frame;
}