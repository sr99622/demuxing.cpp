#pragma once

#include "../Utilities/avexception.h"

class Frame
{
public:
	Frame();
	Frame(const Frame& other);
	~Frame();

	AVFrame* av_frame = NULL;
	AVExceptionHandler av;

	Frame& operator = (const Frame& other)
	{
		if (this != &other) {
			try {
				av.ck(av_frame_ref(av_frame, other.av_frame), CmdTag::AFR);
			}
			catch (const AVException& e) {
				std::cout << "Frame assignment operator exception: " << e.what() << std::endl;
			}
		}
		return *this;
	}
};

