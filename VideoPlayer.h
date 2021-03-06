/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/

#pragma once

#include "OutputVideoPlayer.h"
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

class VideoPlayer
{
public:
	VideoPlayer();
	/**   
	 Initialize the Libav video decoding .
	 <param name = "filename"> Video file name, relative or absolute, local or remote(http) </param>
	 <param name = "device"> Irrlicht device</param>
	 <param name = "scaleToScreenWidth"> true to scale the video frame to the device screen width,
	 false will decode to the native width and height of the video</param>
	 
	 <returns>Returns zero in success.</returns>
	*/
	int init(const char* filename  , IrrlichtDevice *device,bool scaleToScreenWidth = true);
	/** 
	 Decodes a single frame of the video file.should be called after init succeed
	 <returns>Returns zero in success.</returns>
	*/
	int decodeFrame();
	/**
	 Initializes outputting the video file.
	*/
	int beginOutput(const char* filename);
	/**
	 Writes a frame to the output file.should be called after beginOutput succeed
	*/
	int writeFrame(AVFrame* pFrameRGB);
	/**
	 Stops outputting the video file.should be called after beginOutput and writeFrame succeed
	*/
	int endOutput();
	/** 
	returns the native width of the decoded video file. could be called after init succeed
	*/
	int getFrameWidth();
	/**
	returns the native Height of the decoded video file. Should be called after init succeed
	*/
	int getFrameHeight();
	/**
	returns the latest decoded frame as irrlicht ITexture
	*/
	ITexture* getFrameTexture();
	/**
	returns the video FPS
	*/
	int getFrameRate();
	/**
	returns the video length in seconds
	*/
	long getVideoDurationInSeconds();
	~VideoPlayer();

private:
	float framerate = 0.0f;
	int   frameFinished;
	int   numBytes ;
	unsigned char * buffer = NULL;
	long videoDuration;
	int                i, videoStream;
	IVideoDriver* driver;
	IrrlichtDevice *device;
	IImage* imageRt;
	ITexture* frameTexture;
	u32 now, then;
	int frameWidth, frameHeight;
	bool outputting = false;

	int getNearestWidth(float aspect, int height);
	int getNearestHeight(float aspect, int width);
	int decodeFrameInternal();

	OutputVideoPlayer ovp;
	
};

