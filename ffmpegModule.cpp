#include "ffmpegModule.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

void FfmpegModule::saveScreenshot(AVCodecContext* dec_ctx, AVFrame* frame, QString& filename) {
    AVFrame* frameRGB = av_frame_alloc();
    avpicture_alloc((AVPicture*)frameRGB,
                    AV_PIX_FMT_RGB24,
                    dec_ctx->width,
                    dec_ctx->height);
    if(frameRGB == nullptr) {
        return;
    }
    auto convert_ctx = sws_getContext(dec_ctx->width,
                                      dec_ctx->height,
                                      static_cast<AVPixelFormat>(frame->format),
                                      dec_ctx->width,
                                      dec_ctx->height,
                                      AV_PIX_FMT_RGB24,
                                      SWS_BILINEAR, NULL, NULL, NULL);
    if(!convert_ctx)
        return;
   int ret = sws_scale(convert_ctx,
              frame->data,
              frame->linesize,
              0,
              dec_ctx->height,
              frameRGB->data,
              frameRGB->linesize
                        );
    if(ret < 0)
        return;

    QImage image(frameRGB->data[0],
                 dec_ctx->width,
                 dec_ctx->height,
                 frameRGB->linesize[0],
                 QImage::Format_RGB888);
    if(image.Format_Invalid) {
        ERROR("Error decoding!");
        return;
    }

    if(!image.save(filename))
        ERROR("Error saving file!");

    sws_freeContext(convert_ctx);
    avpicture_free((AVPicture*)frameRGB);
}


void FfmpegModule::decodeAudioPacket(AVCodecContext* audioContext, AVPacket* packet) noexcept
{
    AVFrame *aFrame;
    int ret, data_size = 0;

    ret = avcodec_send_packet(audioContext, packet);
    if (ret < 0) {
        LOG_ERROR(1, "ffmpeg: Error submitting the packet to the decoder");
        return;
    }

    while (ret >= 0) {
        aFrame = av_frame_alloc();
        if (!aFrame) {
            WARNING("ffmpeg: Error allocating frame");
            return;
        }
        ret = avcodec_receive_frame(audioContext, aFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&aFrame);
            return;
        }
        else if (ret < 0) {
            ERROR("ffmpeg: Error during decoding");
            av_frame_free(&aFrame);
            return;
        }

        addFrameToQueue(aFrame);
    }
}
