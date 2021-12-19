#pragma once

#include "../Utilities/avexception.h"

class Frame
{
public:
	Frame();
	Frame(const Frame& other);
	~Frame();

	AVMediaType type();
	int width();
	int height();
	int nb_samples();
	uint64_t channel_layout();

	AVFrame* av_frame = NULL;
	AVExceptionHandler av;

	Frame& operator = (const Frame& other)
	{
		if (this != &other) {
			try {
				av.ck(av_frame_copy_props(av_frame, other.av_frame));
				av.ck(av_frame_copy(av_frame, other.av_frame));
			}
			catch (const AVException& e_copy) {
				try {
					av_frame_free(&av_frame);
					av.ck(av_frame = av_frame_clone(other.av_frame));
				}
				catch (const AVException& e_clone) {
					std::cout << "Frame assignment operator exception: " << e_clone.what() << std::endl;
				}
			}
		}
		return *this;
	}
};

