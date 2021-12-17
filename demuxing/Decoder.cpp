#include "Decoder.h"

Decoder::Decoder(AVCodecContext* dec)
{
	dec_ctx = dec;
    next_pts = 0;
    next_pts_tb = av_make_q(0, 1);

	try {
		av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        writer = new RawFileWriter(dec);
	}
	catch (const AVException& e) {
		std::cout << "Decoder constructor exception: " << e.what() << std::endl;
	}
}

Decoder::~Decoder()
{
    if (writer != nullptr)
        delete writer;
    av_frame_free(&frame);
}

int Decoder::decode_packet(AVPacket* pkt) 
{
    int ret = 0;

    try {
        av.ck(ret = avcodec_send_packet(dec_ctx, pkt), CmdTag::ASP);

        while (ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    return 0;
                else
                    throw "error during decoding";
            }
            else {
                adjust_pts(frame);
            }

            writer->write_frame(frame);

            av_frame_unref(frame);
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
}