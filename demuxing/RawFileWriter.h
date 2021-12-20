#pragma once

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#include "../Utilities/avexception.h"
#include "Frame.h"

class RawFileWriter
{
public:
	RawFileWriter(AVCodecContext* codec_ctx);
	~RawFileWriter();

	int write_frame(AVFrame* frame);
	int write_frame(const Frame& f);
	int output_video_frame(AVFrame* frame);
	int output_audio_frame(AVFrame* frame);

	int width;
	int height;
	AVPixelFormat pix_fmt;
	AVMediaType type;

	uint8_t* video_dst_data[4] = { NULL };
	int video_dst_linesize[4];
	int video_dst_bufsize;

	const char* filename;
	FILE* file;
	int frame_count = 0;

	AVExceptionHandler av;
};

