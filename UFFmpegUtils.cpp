#include "pch.h"
#include "UFFmpegUtils.h"
#include <iostream>
#include <fstream>

namespace UFFmpegUtils {

	AVFrame* Convert(AVFrame* srcFrame, AVPixelFormat dstPixFmt, int dstWidth, int dstHeight)
	{
		AVFrame* dstFrame = av_frame_alloc();
		if (!dstFrame) {
			std::cout << "Could not allocate destination frame" << std::endl;
			return nullptr;
		}

		// 设置目标AVFrame的属性
		dstFrame->format = dstPixFmt;
		dstFrame->width = dstWidth > 0 ? dstWidth : srcFrame->width;
		dstFrame->height = dstHeight > 0 ? dstHeight : srcFrame->height;
		int ret = av_frame_get_buffer(dstFrame, 0);
		if (ret < 0) {
			std::cerr << "Could not allocate buffer for destination frame" << std::endl;
			av_frame_free(&dstFrame);
			return nullptr;
		}

		struct SwsContext* sws_ctx = sws_getContext(
			srcFrame->width, srcFrame->height, (AVPixelFormat)srcFrame->format,
			dstFrame->width, dstFrame->height, dstPixFmt,
			SWS_BICUBIC, nullptr, nullptr, nullptr
		);
		if (!sws_ctx) {
			std::cerr << "Could not initialize SwsContext" << std::endl;
			av_frame_free(&dstFrame);
			return nullptr;
		}

		// 执行转换
		sws_scale(
			sws_ctx, (const uint8_t* const*)srcFrame->data, srcFrame->linesize,
			0, srcFrame->height, dstFrame->data, dstFrame->linesize
		);

		sws_freeContext(sws_ctx);
		return dstFrame;
	}

	bool SaveRGB24AsBMP(const AVFrame* rgbFrame, const char* filename) {
		int width = rgbFrame->width;
		int height = rgbFrame->height;
		int padSize = (4 - ((width * 3) % 4)) % 4;
		int rowSize = width * 3 + padSize;
		int imageSize = rowSize * height;

		BITMAPFILEHEADER fileHeader;
		fileHeader.bfType = 0x4D42; // 'BM'
		fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;
		fileHeader.bfReserved1 = 0;
		fileHeader.bfReserved2 = 0;
		fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		BITMAPINFOHEADER infoHeader;
		infoHeader.biSize = sizeof(BITMAPINFOHEADER);
		infoHeader.biWidth = width;
		infoHeader.biHeight = height;
		infoHeader.biPlanes = 1;
		infoHeader.biBitCount = 24;
		infoHeader.biCompression = 0;
		infoHeader.biSizeImage = imageSize;
		infoHeader.biXPelsPerMeter = 0;
		infoHeader.biYPelsPerMeter = 0;
		infoHeader.biClrUsed = 0;
		infoHeader.biClrImportant = 0;

		std::ofstream outputFile(filename, std::ios::binary);
		if (!outputFile) {
			return false;
		}
		outputFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
		outputFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

		uint8_t* data = rgbFrame->data[0];
		for (int y = height - 1; y >= 0; --y) {
			outputFile.write(reinterpret_cast<const char*>(data + y * rgbFrame->linesize[0]), width * 3);
			if (padSize > 0) {
				char padding[3] = { 0 };
				outputFile.write(padding, padSize);
			}
		}

		outputFile.close();
		return true;
	}
	
}