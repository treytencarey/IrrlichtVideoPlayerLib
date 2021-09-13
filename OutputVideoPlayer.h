/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/

#pragma once

#include <irrlicht.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
}
#include <stdio.h>
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif
#include <iostream>

using namespace irr;
using namespace video;
using namespace core;

class OutputVideoPlayer
{
public:
	OutputVideoPlayer();
	
	void pushFrame(AVFrame* pFrame);
	void finish();
	void free();
	int init();

private:
	AVFrame* videoFrame = nullptr;
	AVCodecContext* cctx = nullptr;
	SwsContext* swsCtx = nullptr;
	int frameCounter = 0;
	AVFormatContext* ofctx = nullptr;
	AVOutputFormat* oformat = nullptr;
	int fps = 30;
	int width = 1920;
	int height = 1080;
	int bitrate = 20;

};

