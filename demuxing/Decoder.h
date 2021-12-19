#pragma once

#include "../Utilities/avexception.h"
#include "RawFileWriter.h"
#include "CircularQueue.h"

class Decoder
{
public:
	Decoder(AVFormatContext* fmt_ctx, int stream_index, CircularQueue<AVFrame*>* q);
	~Decoder();
	int decode_packet(AVPacket* pkt);
	void adjust_pts(AVFrame* frame);
	void flush();

	AVFrame* frame = NULL;
	AVCodecContext* dec_ctx;
	CircularQueue<AVFrame*>* frame_q;
	AVExceptionHandler av;

	AVRational next_pts_tb;
	int64_t next_pts;
};

