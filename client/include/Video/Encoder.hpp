#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class Encoder
{
    public:
        Encoder();
        ~Encoder();

        bool open(int width, int height, int fps);
        std::vector<uint8_t> encodeFrame(const uint8_t* pixels, int width, int height);
        void close();

    private:
        AVCodecContext* mCodecCtx = nullptr;
        AVFrame*        mFrame    = nullptr;
        AVPacket*       mPacket   = nullptr;
        SwsContext*     mSwsCtx   = nullptr;

        int      mWidth     = 0;
        int      mHeight    = 0;
        int64_t  mFrameIdx  = 0;
        bool     mOpen      = false;

        struct AVFormatContext* mFmtCtx = nullptr;
        struct AVStream*        mStream = nullptr;
};