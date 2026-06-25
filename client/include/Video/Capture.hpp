#pragma once

#include "Video/Frame.hpp"

class Capture
{
    public:
        Capture(int width, int height);

        Frame capture(const uint32_t* framebuffer,
                        uint64_t frameNumber);

    private:
        int mWidth;
        int mHeight;
};