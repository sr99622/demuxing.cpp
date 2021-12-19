#include "Frame.h"

Frame::Frame()
{
	av.ck(av_frame = av_frame_alloc(), CmdTag::AFA);
}

Frame::Frame(const Frame& other)
{
	av.ck(av_frame_ref(av_frame, other.av_frame), CmdTag::AFR);
}

Frame::~Frame()
{
	av_frame_free(&av_frame);
}

AVMediaType Frame::type()
{
	AVMediaType result = AVMEDIA_TYPE_UNKNOWN;

	if (width() > 0 && height() > 0)
		result = AVMEDIA_TYPE_VIDEO;
	else if (nb_samples() > 0 && channel_layout())
		result = AVMEDIA_TYPE_AUDIO;

	return result;
}

int Frame::width()
{
	return av_frame->width;
}

int Frame::height()
{
	return av_frame->height;
}

int Frame::nb_samples()
{
	return av_frame->nb_samples;
}

uint64_t Frame::channel_layout()
{
	return av_frame->channel_layout;
}