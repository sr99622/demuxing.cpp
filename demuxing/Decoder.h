#pragma once

#include "../Utilities/avexception.h"
#include "RawFileWriter.h"

class Decoder
{
public:
	Decoder(AVCodecContext* dec);
	~Decoder();
	int decode_packet(AVPacket* pkt);
	void flush();
	void adjust_pts(AVFrame* frame);

	AVCodecContext* dec_ctx;
	AVFrame* frame;
	RawFileWriter* writer = nullptr;
	AVExceptionHandler av;

	AVRational next_pts_tb;
	int64_t next_pts;
};

