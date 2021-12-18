#pragma once

#include "../Utilities/avexception.h"
#include "RawFileWriter.h"

class Decoder
{
public:
	Decoder(AVFormatContext* fmt_ctx, int stream_index);
	~Decoder();
	AVFrame* decode_packet(AVPacket* pkt);
	void adjust_pts(AVFrame* frame);
	void flush();

	AVCodecContext* dec_ctx;
	RawFileWriter* writer = nullptr;
	AVExceptionHandler av;

	AVRational next_pts_tb;
	int64_t next_pts;
};

