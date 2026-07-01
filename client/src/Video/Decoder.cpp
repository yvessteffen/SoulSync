#define NOMINMAX
#include "Video/Decoder.hpp"
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

Decoder::Decoder() {}

Decoder::~Decoder()
{
    close();
}

bool Decoder::open()
{
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        std::cerr << "H.264 decoder not found\n";
        return false;
    }

    mCodecCtx = avcodec_alloc_context3(codec);
    if (!mCodecCtx) return false;

    if (avcodec_open2(mCodecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Could not open decoder\n";
        return false;
    }

    mFrame     = av_frame_alloc();
    mRGBAFrame = av_frame_alloc();
    mPacket    = av_packet_alloc();
    mOpen      = true;
    return true;
}

bool Decoder::decodePacket(const uint8_t* data, int size,
                            std::vector<uint8_t>& outPixels,
                            int& outWidth, int& outHeight)
{
    if (!mOpen) return false;

    mPacket->data = const_cast<uint8_t*>(data);
    mPacket->size = size;

    int ret = avcodec_send_packet(mCodecCtx, mPacket);
    if (ret < 0)
    {
        char err[64];
        av_strerror(ret, err, sizeof(err));
        std::cerr << "Error sending packet: " << err << "\n";
        return false;
    }

    ret = avcodec_receive_frame(mCodecCtx, mFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        return false;
    if (ret < 0)
    {
        char err[64];
        av_strerror(ret, err, sizeof(err));
        std::cerr << "Error receiving frame: " << err << "\n";
        return false;
    }

    outWidth  = mFrame->width;
    outHeight = mFrame->height;

    if (!mSwsCtx)
    {
        mSwsCtx = sws_getContext(
            mFrame->width, mFrame->height,
            (AVPixelFormat)mFrame->format,
            mFrame->width, mFrame->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
    }

    outPixels.resize(mFrame->width * mFrame->height * 4);
    uint8_t* dst[1]   = { outPixels.data() };
    int dstStride[1]  = { mFrame->width * 4 };

    sws_scale(mSwsCtx,
              mFrame->data, mFrame->linesize,
              0, mFrame->height,
              dst, dstStride);

    return true;
}

void Decoder::close()
{
    if (!mOpen) return;
    mOpen = false;
    avcodec_free_context(&mCodecCtx);
    av_frame_free(&mFrame);
    av_frame_free(&mRGBAFrame);
    av_packet_free(&mPacket);
    if (mSwsCtx) sws_freeContext(mSwsCtx);
    mSwsCtx = nullptr;
}