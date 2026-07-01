#pragma once
#include <vector>
#include <cstdint>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class Decoder
{
public:
    Decoder();
    ~Decoder();

    bool open();

    bool decodePacket(const uint8_t* data, int size,
                      std::vector<uint8_t>& outPixels,
                      int& outWidth, int& outHeight);
    void close();

private:
    AVCodecContext* mCodecCtx = nullptr;
    AVFrame*        mFrame    = nullptr;
    AVFrame*        mRGBAFrame = nullptr;
    AVPacket*       mPacket   = nullptr;
    SwsContext*     mSwsCtx   = nullptr;
    bool            mOpen     = false;
};