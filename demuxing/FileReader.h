#pragma once

#include "..\Utilities\avexception.h"

class FileReader
{
public:
	FileReader(const char* filename);
	~FileReader();
	int open_codec_context(AVCodecContext** dec_ctx, AVMediaType type, AVStream** stream);
	int open_video_context();
	int open_audio_context();
	AVPacket* read_packet();

	const char* src_filename = "news.mp4";
	AVFormatContext* fmt_ctx;
	//AVPacket* pkt = NULL;
	AVCodecContext* video_dec_ctx = NULL;
	AVCodecContext* audio_dec_ctx = NULL;
	AVStream* video_stream = NULL;
	AVStream* audio_stream = NULL;
	int video_stream_idx = -1;
	int audio_stream_idx = -1;

	AVExceptionHandler av;
};

