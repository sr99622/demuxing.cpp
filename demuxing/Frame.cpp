#include "Frame.h"

Frame::Frame()
{
 	av.ck(av_frame = av_frame_alloc(), CmdTag::AFA);
}

Frame::Frame(const Frame& other)
{
	av.ck(av_frame = av_frame_clone(other.av_frame), "av_frame_clone");
}

Frame::~Frame()
{
	av_frame_free(&av_frame);
}
