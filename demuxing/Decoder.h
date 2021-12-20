#pragma once

#include "../Utilities/avexception.h"
#include "RawFileWriter.h"
#include "CircularQueue.h"
#include "Frame.h"

class Decoder
{
public:
	Decoder(AVFormatContext* fmt_ctx, int stream_index, CircularQueue<Frame>* q);
	~Decoder();
	int decode_packet(AVPacket* pkt);
	void adjust_pts(AVFrame* frame);
	void flush();

	AVFrame* frame = NULL;
	AVCodecContext* dec_ctx;
	CircularQueue<Frame>* frame_q;
	AVExceptionHandler av;
	Frame tmp;
	AVRational next_pts_tb;
	int64_t next_pts;
};

