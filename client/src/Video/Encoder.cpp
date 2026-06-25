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

bool Encoder::open(const std::string& outputPath, int width, int height, int fps)
{
    mWidth  = width;
    mHeight = height;

    avformat_alloc_output_context2(&mFmtCtx, nullptr, nullptr, outputPath.c_str());
    if (!mFmtCtx)
    {
        std::cerr << "Could not allocate format context\n";
        return false;
    }
    std::cout << "Format: " << mFmtCtx->oformat->name << "\n";
    std::cout << "Output: " << outputPath << "\n";

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        std::cerr << "H.264 encoder not found\n";
        return false;
    }

    mStream = avformat_new_stream(mFmtCtx, codec);
    if (!mStream)
    {
        std::cerr << "Could not create stream\n";
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
    mCodecCtx->gop_size = fps * 2;

    av_opt_set(mCodecCtx->priv_data, "crf", "16", 0);
    av_opt_set(mCodecCtx->priv_data, "profile", "high444", 0);
    av_opt_set(mCodecCtx->priv_data, "tune", "animation", 0);

    if (mFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        mCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(mCodecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Could not open codec\n";
        return false;
    }

    avcodec_parameters_from_context(mStream->codecpar, mCodecCtx);
    mStream->time_base = mCodecCtx->time_base;

    if (!(mFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&mFmtCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            std::cerr << "Could not open output file\n";
            return false;
        }
    }

    if (avformat_write_header(mFmtCtx, nullptr) < 0)
    {
        std::cerr << "Could not write header\n";
        return false;
    }

    mFrame = av_frame_alloc();
    mFrame->format = AV_PIX_FMT_YUV444P;
    mFrame->width  = width;
    mFrame->height = height;
    av_frame_get_buffer(mFrame, 0);

    mPacket = av_packet_alloc();

    mSwsCtx = sws_getContext(
        width, height, AV_PIX_FMT_RGB0,
        width, height, AV_PIX_FMT_YUV444P,
        SWS_POINT, nullptr, nullptr, nullptr
    );

    mOpen     = true;
    mFrameIdx = 0;
    std::cout << "VideoEncoder opened: " << outputPath << "\n";
    return true;
}

void Encoder::encodeFrame(const uint8_t* pixels, int width, int height)
{
    if (!mOpen) return;

    const uint8_t* srcSlice[1] = { pixels };
    int srcStride[1] = { 256 * 4 };    
    sws_scale(mSwsCtx, srcSlice, srcStride, 0, height,
              mFrame->data, mFrame->linesize);

    mFrame->pts = mFrameIdx++;

    int ret = avcodec_send_frame(mCodecCtx, mFrame);
    if (ret < 0)
    {
        char err[64];
        av_strerror(ret, err, sizeof(err));
        std::cerr << "Error sending frame: " << err << "\n";
        return;
    }

    while ((ret = avcodec_receive_packet(mCodecCtx, mPacket)) == 0)
    {
        static int pktCount = 0;
        if (++pktCount <= 3)
            std::cout << "packet: pts=" << mPacket->pts 
                      << " size=" << mPacket->size << "\n";

        av_packet_rescale_ts(mPacket, mCodecCtx->time_base, mStream->time_base);
        mPacket->stream_index = mStream->index;
        int writeRet = av_interleaved_write_frame(mFmtCtx, mPacket);
        if (writeRet < 0)
        {
            char err[64];
            av_strerror(writeRet, err, sizeof(err));
            std::cerr << "Error writing packet: " << err << "\n";
        }
        av_packet_unref(mPacket);
    }

    if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        char err[64];
        av_strerror(ret, err, sizeof(err));
        std::cerr << "Error receiving packet: " << err << "\n";
    }
}

void Encoder::close()
{
    std::cout << "VideoEncoder closing, frames encoded: " << mFrameIdx << "\n";
    if (!mOpen) return;
    mOpen = false;

    avcodec_send_frame(mCodecCtx, nullptr);
    while (avcodec_receive_packet(mCodecCtx, mPacket) == 0)
    {
        av_packet_rescale_ts(mPacket, mCodecCtx->time_base, mStream->time_base);
        mPacket->stream_index = mStream->index;
        av_interleaved_write_frame(mFmtCtx, mPacket);
        av_packet_unref(mPacket);
    }

    av_write_trailer(mFmtCtx);

    avcodec_free_context(&mCodecCtx);
    av_frame_free(&mFrame);
    av_packet_free(&mPacket);
    sws_freeContext(mSwsCtx);

    if (!(mFmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&mFmtCtx->pb);

    avformat_free_context(mFmtCtx);
    mFmtCtx  = nullptr;
    mSwsCtx  = nullptr;
    mStream  = nullptr;
}