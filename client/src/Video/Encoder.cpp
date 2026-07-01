#define NOMINMAX
#include "Video/Encoder.hpp"
#include "Video/Frame.hpp"
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

Encoder::Encoder() {}

Encoder::~Encoder()
{
    close();
}

bool Encoder::open(int width, int height, int fps)
{
    mWidth  = width;
    mHeight = height;

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        std::cerr << "H.264 encoder not found\n";
        return false;
    }

    mCodecCtx = avcodec_alloc_context3(codec);
    if (!mCodecCtx)
    {
        std::cerr << "Could not allocate codec context\n";
        return false;
    }

    mCodecCtx->width     = width;
    mCodecCtx->height    = height;
    mCodecCtx->time_base = { 1, fps };
    mCodecCtx->framerate = { fps, 1 };
    mCodecCtx->pix_fmt   = AV_PIX_FMT_YUV444P;
    mCodecCtx->gop_size  = fps * 2;

    av_opt_set(mCodecCtx->priv_data, "crf",     "16",        0);
    av_opt_set(mCodecCtx->priv_data, "profile", "high444",   0);
    av_opt_set(mCodecCtx->priv_data, "tune",    "animation", 0);

    int ret = avcodec_open2(mCodecCtx, codec, nullptr);
    if (ret < 0)
    {
        char err[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err, sizeof(err));
        std::cerr << err << '\n';
        return false;
    }

    mFrame = av_frame_alloc();
    mFrame->format = AV_PIX_FMT_YUV444P;
    mFrame->width  = width;
    mFrame->height = height;
    if (av_frame_get_buffer(mFrame, 0) < 0)
    {
        std::cerr << "Could not allocate frame buffer\n";
        return false;
    }

    mPacket = av_packet_alloc();

    mSwsCtx = sws_getContext(
        width, height, AV_PIX_FMT_RGB0,
        width, height, AV_PIX_FMT_YUV444P,
        SWS_POINT, nullptr, nullptr, nullptr
    );

    mOpen     = true;
    mFrameIdx = 0;
    return true;
}

std::vector<uint8_t> Encoder::encodeFrame(const uint8_t* pixels, int width, int height)
{
    std::vector<uint8_t> result;
    if (!mOpen) return result;

    const uint8_t* srcSlice[1] = { pixels };
    int srcStride[1] = { 256 * 4 };
    sws_scale(mSwsCtx, srcSlice, srcStride, 0, height,
              mFrame->data, mFrame->linesize);

    mFrame->pts = mFrameIdx++;

    if (avcodec_send_frame(mCodecCtx, mFrame) < 0)
        return result;

    while (avcodec_receive_packet(mCodecCtx, mPacket) == 0)
    {
        size_t offset = result.size();
        result.resize(offset + mPacket->size);
        std::memcpy(result.data() + offset, mPacket->data, mPacket->size);
        av_packet_unref(mPacket);
    }

    return result;
}

void Encoder::close()
{
    std::cout << "VideoEncoder closing, frames encoded: " << mFrameIdx << "\n";
    if (!mOpen) return;
    mOpen = false;

    avcodec_send_frame(mCodecCtx, nullptr);
    while (avcodec_receive_packet(mCodecCtx, mPacket) == 0)
    {
        av_packet_unref(mPacket);
    }

    avcodec_free_context(&mCodecCtx);
    av_frame_free(&mFrame);
    av_packet_free(&mPacket);
    sws_freeContext(mSwsCtx);

    mSwsCtx  = nullptr;
}