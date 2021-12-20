#include "RawFileWriter.h"

RawFileWriter::RawFileWriter(AVCodecContext* codec_ctx)
{
	if (codec_ctx->codec->type == AVMEDIA_TYPE_VIDEO) {
		width = codec_ctx->width;
		height = codec_ctx->height;
		pix_fmt = codec_ctx->pix_fmt;

		try {
			av.ck(video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1), CmdTag::AIA);
		}
		catch (const AVException& e) {
			std::cout << "RawFileWriter constructor exception: " << e.what() << std::endl;
		}

        type = AVMEDIA_TYPE_VIDEO;
        filename = "video";
	}
    else {
        type = AVMEDIA_TYPE_AUDIO;
        filename = "audio";
    }

    file = fopen(filename, "wb");
    if (!file) {
        std::cout << "Error opening file " << filename << std::endl;
     }

}

RawFileWriter::~RawFileWriter()
{
    if (file)
        fclose(file);

    av_free(video_dst_data[0]);
}

int RawFileWriter::write_frame(const Frame& f) {
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

int RawFileWriter::output_video_frame(AVFrame* frame)
{
    if (frame->width != width || frame->height != height || frame->format != pix_fmt) {
        std::cout << "RawFileWriter output_video_frame error: inconsistent format" << std::endl;
        return -1;
    }

    printf("video_frame n:%d coded_n:%d\n", frame_count++, frame->coded_picture_number);

    av_image_copy(video_dst_data, video_dst_linesize,
        (const uint8_t**)(frame->data), frame->linesize,
        pix_fmt, width, height);

    fwrite(video_dst_data[0], 1, video_dst_bufsize, file);
    return 0;
}

int RawFileWriter::output_audio_frame(AVFrame* frame)
{
    size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);

    fwrite(frame->extended_data[0], 1, unpadded_linesize, file);

    return 0;
}
