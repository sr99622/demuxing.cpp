#include "RawFileWriter.h"

RawFileWriter::RawFileWriter(AVCodecContext* dec_ctx)
{
    try {
        type = dec_ctx->codec->type;

        if (type == AVMEDIA_TYPE_VIDEO) {
            width = dec_ctx->width;
            height = dec_ctx->height;
            pix_fmt = dec_ctx->pix_fmt;

            size = av_image_get_buffer_size((AVPixelFormat)pix_fmt, width, height, 1);
            av.ck(buffer = (uint8_t*)av_malloc(size), CmdTag::AM);

            filename = "video";
        }
        else if (type == AVMEDIA_TYPE_AUDIO) {
            filename = "audio";
        }
        else {
            throw AVException("unsupported media type");
        }

        file = fopen(filename, "wb");
        if (!file) {
            throw AVException(std::string("Error opening file ") +  filename + ": " + strerror(errno));
        }
    }
    catch (const AVException& e) {
        std::cout << "RawFileWriter constructor exception: " << e.what() << std::endl;
    }
}

RawFileWriter::~RawFileWriter()
{
    if (file)
        fclose(file);

    av_freep(&buffer);
}

int RawFileWriter::write_frame(const Frame& f)
{
    return write_frame(f.av_frame);
}

int RawFileWriter::write_frame(AVFrame* frame)
{
    int result = -1;
    if (type == AVMEDIA_TYPE_VIDEO) {
        result = output_video_frame(frame);
    }
    else if (type == AVMEDIA_TYPE_AUDIO) {
        result = output_audio_frame(frame);
    }
    return result;
}

int RawFileWriter::output_audio_frame(AVFrame* frame)
{
    int ret = 0;

    try {
        size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);

        if (fwrite(frame->extended_data[0], 1, unpadded_linesize, file) != unpadded_linesize) {
            ret = -1;
            throw AVException(std::string("fwrite failed: ") + strerror(errno));
        }
    }
    catch (const AVException& e) {
        std::cout << "RawFileWriter::output_audio_frame exception: " << e.what() << std::endl;
    }

    return ret;
}

int RawFileWriter::output_video_frame(AVFrame* frame)
{
    int ret;

    try {
        av.ck(ret = av_image_copy_to_buffer(buffer, size,
            (const uint8_t* const*)frame->data,
            (const int*)frame->linesize, (AVPixelFormat)frame->format,
            frame->width, frame->height, 1), CmdTag::AICTB);

        if (fwrite(buffer, 1, size, file) != size) {
            ret = -1;
            throw AVException(std::string("fwrite failed: ") + strerror(errno));
        }
    }
    catch (const AVException& e) {
        std::cout << "RawFileWriter::output_vide_frame exception: " << e.what() << std::endl;
    }

    return ret;
}