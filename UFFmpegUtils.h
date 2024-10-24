#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif

namespace UFFmpegUtils {

	// resize and color space conversion
	AVFrame* Convert(AVFrame* srcFrame, AVPixelFormat dstPixFmt, int dstWidth = -1, int dstHeight = -1);

	bool SaveRGB24AsBMP(const AVFrame* rgbFrame, const char* filename);

}