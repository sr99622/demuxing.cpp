#include "Decoder.h"

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts)
{
    const AVPixelFormat* p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

Decoder::Decoder(AVFormatContext* fmt_ctx, int stream_index, CircularQueue<Frame>* q, AVHWDeviceType type)
{
    next_pts = 0;
    next_pts_tb = av_make_q(0, 1);
    frame_q = q;

    try {
        av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        AVStream* stream = fmt_ctx->streams[stream_index];
        const AVCodec* dec = NULL;
        av.ck(dec = avcodec_find_decoder(stream->codecpar->codec_id), std::string("avcodec_find_decoder could not find ") + avcodec_get_name(stream->codecpar->codec_id));
        av.ck(dec_ctx = avcodec_alloc_context3(dec), CmdTag::AAC3);
        av.ck(avcodec_parameters_to_context(dec_ctx, stream->codecpar), CmdTag::APTC);

        if (type != AV_HWDEVICE_TYPE_NONE) {
            av.ck(sw_frame = av_frame_alloc(), CmdTag::AFA);
            for (int i = 0;; i++) {
                const AVCodecHWConfig* config;
                av.ck(config = avcodec_get_hw_config(dec, i), std::string("Decoder ") + dec->name + std::string(" does not support device type ") + av_hwdevice_get_type_name(type));

                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
                    hw_pix_fmt = config->pix_fmt;
                    break;
                }
            }
            dec_ctx->get_format = get_hw_format;
            av.ck(av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0), CmdTag::AHCC);
            dec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        }

        av.ck(avcodec_open2(dec_ctx, dec, NULL), CmdTag::AO2);
    }
    catch (const AVException& e) {
        std::cout << "Decoder constructor exception: " << e.what() << std::endl;
    }
}

Decoder::~Decoder()
{
    av_frame_unref(frame);
    if (sw_frame)
        av_frame_unref(sw_frame);
    avcodec_free_context(&dec_ctx);
    av_buffer_unref(&hw_device_ctx);
}

int Decoder::decode_packet(AVPacket* pkt)
{
    int ret = 0;

    try {
        av.ck(ret = avcodec_send_packet(dec_ctx, pkt), CmdTag::ASP);

        while (ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    return 0;
                }
                else if (ret < 0) {
                    throw AVException("error during decoding");
                }
            }
            else {
                adjust_pts(frame);
            }

            if (frame->format == hw_pix_fmt) {
                av.ck(ret = av_hwframe_transfer_data(sw_frame, frame, 0), CmdTag::AHTD);
                tmp.av_frame = sw_frame;
            }
            else {
                tmp.av_frame = frame;
            }

            frame_q->push(tmp);
        }
    }
    catch (const AVException& e) {
        std::cout << "Decoder::decode_packet exception: " << e.what() << std::endl;
    }

    return ret;
}

void Decoder::adjust_pts(AVFrame* frame)
{
    switch (dec_ctx->codec_type) {

    case AVMEDIA_TYPE_VIDEO:
        frame->pts = frame->pkt_dts;
        break;

    case AVMEDIA_TYPE_AUDIO:
        AVRational tb = av_make_q(1, frame->sample_rate);

        if (frame->pts != AV_NOPTS_VALUE) {
            frame->pts = av_rescale_q(frame->pts, dec_ctx->pkt_timebase, tb);
        }
        else if (next_pts != AV_NOPTS_VALUE) {
            frame->pts = av_rescale_q(next_pts, next_pts_tb, tb);
        }

        if (frame->pts != AV_NOPTS_VALUE) {
            next_pts = frame->pts + frame->nb_samples;
            next_pts_tb = tb;
        }
        break;
    }
}

void Decoder::flush()
{
    decode_packet(NULL);
    avcodec_flush_buffers(dec_ctx);
    frame_q->flush();
}