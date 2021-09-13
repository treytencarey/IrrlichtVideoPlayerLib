/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/
#pragma warning(disable : 4996)

#include "VideoPlayer.h"


// Initalizing these to NULL prevents segfaults!
AVFormatContext   *pFormatCtx = NULL;
AVCodecContext    *pCodecCtxOrig = NULL;
AVCodecContext    *pCodecCtx = NULL;
AVCodec           *pCodec = NULL;
AVFrame           *pFrame = NULL;
AVFrame           *pFrameRGB = NULL;
AVPacket          packet;

AVOutputFormat*   pOutFormat;
AVFormatContext*  pOutFormatCtx = NULL;
AVCodecContext*   pOutCodecCtx = NULL;
AVCodec*          pOutCodec = NULL;
AVFrame*          pOutFrame = NULL;
AVPacket		  outPacket;
AVStream*		  outStream;

struct SwsContext *sws_ctx = NULL;


VideoPlayer::VideoPlayer()
{
	frameTexture = NULL;
}

int VideoPlayer::getFrameRate() {
	return framerate;
}

long VideoPlayer::getVideoDurationInSeconds() {
	return videoDuration;
}
/// Copies data from the decoded Frame to the irrlicht image
void writeVideoTexture(AVFrame *pFrame, IImage* imageRt) {
	
	if (pFrame->data[0] == NULL) {
		return;
	}

	int width = imageRt->getDimension().Width;
	int height = imageRt->getDimension().Height;

	//from big Indian to little indian if needed
	int i, temp;
	uint8_t* frameData = pFrame->data[0];

	//for (i = 0; i < width*height * 3; i += 3)
	//{
		//swap R and B; raw_image[i + 1] is G, so it stays where it is.

		//temp = frameData[i + 0];
		//frameData[i + 0] = frameData[i + 2];
		//frameData[i + 2] = temp;
	//}
	unsigned char* data = (unsigned char*)(imageRt->lock());
	//Decoded format is R8G8B8
	memcpy(data, frameData, width * height * 3);
	imageRt->unlock();
}

ITexture* VideoPlayer::getFrameTexture() {
	driver->removeTexture(frameTexture);
	frameTexture = driver->addTexture("tmp", imageRt);
	return  frameTexture;
}

static const std::string av_make_error_string(int errnum)
{
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
	return (std::string)errbuf;
}

int VideoPlayer::init(const char* filename, IrrlichtDevice *device, bool scaleToScreenWidth) {
	
	this->device = device;
	driver = device->getVideoDriver();

	avdevice_register_all();
	av_log_set_level(AV_LOG_DEBUG);

	//Meta-Data,not used
	AVDictionaryEntry *tag = NULL;

	AVInputFormat* format = NULL;
	if (filename == "desktop")
		format = (AVInputFormat*)av_find_input_format("gdigrab");

	// Open video file
	if (avformat_open_input(&pFormatCtx, filename, format, NULL) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return -1; // Couldn't find stream information

	  // Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, filename, 0);
	//Duration of the video file
	videoDuration = (pFormatCtx->duration / AV_TIME_BASE);
	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	  // Get a pointer to the codec context for the video stream
	// pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = (AVCodec*)avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);
	/*if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}*/

	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

	if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_opt_set(pCodecCtx, "preset", "ultrafast", 0);
	}
	else if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
		av_opt_set(pCodecCtx, "preset", "ultrafast", 0);
	}
	else
	{
		av_opt_set_int(pCodecCtx, "lossless", 1, 0);
	}

	// Open codec
	int ret;
	if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0)
	{
		std::cout << av_make_error_string(ret) << std::endl;
		return -1; // Could not open codec
	}

	  // Allocate video frame
	pFrame = av_frame_alloc();

	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;
	//saving decoded width,height data
	frameWidth = pCodecCtx->width;
	frameHeight = pCodecCtx->height;

	int width = scaleToScreenWidth?driver->getScreenSize().Width : pCodecCtx->width;
	int height = scaleToScreenWidth ? getNearestHeight(((float)pCodecCtx->width) / pCodecCtx->height, width): 
		pCodecCtx->height;
	// Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
		width, height, 1);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		width,
		height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	//Frame Rate of the file
	framerate = filename == "desktop" ? 15 : pFormatCtx->streams[videoStream]->r_frame_rate.num;
	//Creating the image holding the decoded video frame data
	imageRt = driver->createImage(ECOLOR_FORMAT::ECF_R8G8B8,
		dimension2du(width, height));
	
	avcodec_flush_buffers(pCodecCtx);
	then = device->getTimer()->getTime();

	ovp.init();

	return 0;
}

int VideoPlayer::decodeFrame() {
	i = 0;
	while (true) {
		now = device->getTimer()->getTime();
		//Decode next frame only if the current finished. Weak sync method ,to be enhanced later
		if ((now - then) >= (1. / (framerate + 1.5f)) * 1000) {
			if ((i = decodeFrameInternal()) != 0) {
				if (frameTexture)
					driver->removeTexture(frameTexture);				
				continue;
			}
			then = now;			
		}
		break;
	}
	return i;
}

int VideoPlayer::decodeFrameInternal() {
	int ret = 0;
	if (ret = av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			// avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			avcodec_send_packet(pCodecCtx, &packet);
			while (ret >= 0) {
				ret = avcodec_receive_frame(pCodecCtx, pFrame);
				if (ret == AVERROR(EAGAIN))
				{
					ret = 0;
					avcodec_send_packet(pCodecCtx, &packet);
					break;
				}
				
				//pFormatCtx->duration
				// Convert the image from its native format to RGB
				sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				writeVideoTexture(pFrameRGB, imageRt);

				if (outputting)
				{
					// writeFrame(pFrameRGB);
				}

				ovp.pushFrame(pFrame);
			}
			ret = 0;
		}
		// Free the packet that was allocated by av_read_frame
		av_packet_unref(&packet);
	}
	return ret;
}

int VideoPlayer::beginOutput(const char* filename)
{
	pOutFormatCtx = NULL;
	int value = 0;

	/* Returns the output format in the list of registered output formats which best matches the provided parameters, or returns NULL if there is no match. */
	pOutFormat = (AVOutputFormat*)av_guess_format(NULL, filename, NULL);
	if (!pOutFormat)
	{
		std::cout << "\nerror in guessing the video format. try with correct format";
		exit(1);
	}

	avformat_alloc_output_context2(&pOutFormatCtx, pOutFormat, NULL, filename);
	if (!pOutFormatCtx)
	{
		std::cout << "\nerror in allocating av format output context";
		exit(1);
	}

	pOutCodec = (AVCodec*)avcodec_find_encoder(pOutFormat->video_codec);
	if (!pOutCodec)
	{
		std::cout << "\nerror in finding the av codecs. try again with correct codec";
		exit(1);
	}

	outStream = avformat_new_stream(pOutFormatCtx, NULL);
	if (!outStream)
	{
		std::cout << "\nerror in creating a av format new stream";
		exit(1);
	}

	pOutCodecCtx = avcodec_alloc_context3(pOutCodec);
	if (!pOutCodecCtx)
	{
		std::cout << "\nerror in allocating the codec contexts";
		exit(1);
	}

	/* set property of the video file */
	int bitrate = 20;
	int fps = 30;

	outStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	outStream->codecpar->width = 1920;
	outStream->codecpar->height = 1080;
	outStream->codecpar->format = AV_PIX_FMT_YUV420P;
	outStream->codecpar->bit_rate = bitrate * 1000;
	outStream->avg_frame_rate = { fps, 1 };
	avcodec_parameters_to_context(pOutCodecCtx, outStream->codecpar);
	pOutCodecCtx->time_base = { 1, 1 };
	pOutCodecCtx->max_b_frames = 2;
	pOutCodecCtx->gop_size = 12;
	pOutCodecCtx->framerate = { fps, 1 };

	if (pOutCodecCtx->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(pOutCodecCtx->priv_data, "preset", "slow", 0);
	}

	avcodec_parameters_from_context(outStream->codecpar, pOutCodecCtx);

	/* Some container formats (like MP4) require global headers to be present
	   Mark the encoder so that it behaves accordingly. */

	if (pOutFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		pOutFormatCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	value = avcodec_open2(pOutCodecCtx, pOutCodec, NULL);
	if (value < 0)
	{
		std::cout << "\nerror in opening the avcodec";
		exit(1);
	}

	/* create empty video file */
	if (!(pOutFormatCtx->flags & AVFMT_NOFILE))
	{
		if (avio_open2(&pOutFormatCtx->pb, filename, AVIO_FLAG_WRITE, NULL, NULL) < 0)
		{
			std::cout << "\nerror in creating the video file";
			exit(1);
		}
	}

	if (!pOutFormatCtx->nb_streams)
	{
		std::cout << "\noutput file dose not contain any stream";
		exit(1);
	}

	/* imp: mp4 container or some advanced container file required header information*/
	value = avformat_write_header(pOutFormatCtx, NULL);
	if (value < 0)
	{
		std::cout << "\nerror in writing the header context: " << av_make_error_string(value);
		exit(1);
	}

	/*
	// uncomment here to view the complete video file informations
	cout<<"\n\nOutput file information :\n\n";
	av_dump_format(outAVFormatContext , 0 ,output_file ,1);
	*/
	outputting = true;

	av_dump_format(pOutFormatCtx, 0, filename, 1);

	return 0;
}

int VideoPlayer::writeFrame(AVFrame* pFrame)
{
	if (!outputting)
		return -1;

	pOutFrame = av_frame_alloc();
	outPacket = *av_packet_alloc();
	outPacket.data = NULL;
	outPacket.size = 0;

	int ret = 0;
	if (ret = av_read_frame(pOutFormatCtx, &outPacket) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			// avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			avcodec_send_frame(pOutCodecCtx, pOutFrame);
			while (ret >= 0) {
				ret = avcodec_receive_packet(pOutCodecCtx, &outPacket);
				if (ret == AVERROR(EAGAIN))
				{
					std::cout << "writeFrame: " << av_make_error_string(ret) << std::endl;
					avcodec_send_frame(pOutCodecCtx, pOutFrame);
					return -1;
				}

				if (outPacket.pts != AV_NOPTS_VALUE)
					outPacket.pts = av_rescale_q(outPacket.pts, outStream->time_base, outStream->time_base);
				if (outPacket.dts != AV_NOPTS_VALUE)
					outPacket.dts = av_rescale_q(outPacket.dts, outStream->time_base, outStream->time_base);

				static int counter = 0;
				if (counter == 0) {
					FILE* fp = fopen("dump_first_frame1.dat", "wb");
					fwrite(outPacket.data, outPacket.size, 1, fp);
					fclose(fp);
				}
				std::cout << "pkt key: " << (outPacket.flags & AV_PKT_FLAG_KEY) << " " <<
					outPacket.size << " " << (counter++) << std::endl;
				uint8_t* size = ((uint8_t*)outPacket.data);
				std::cout << "first: " << (int)size[0] << " " << (int)size[1] <<
					" " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] <<
					" " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7] <<
					std::endl;

				if (av_write_frame(pOutFormatCtx, &outPacket) != 0)
				{
					std::cout << "\nerror in writing video frame";
				}

				av_packet_unref(&outPacket);
			}
		}
	}

	return 0;
}

int VideoPlayer::endOutput()
{
	return 0;
}


/**
Nearest width preserving aspect
*/
int VideoPlayer::getNearestWidth(float aspect, int height) {
	return height * aspect;
}
/**
Nearest Height preserving aspect
*/
int VideoPlayer::getNearestHeight(float aspect, int width) {
	return width * (1. / aspect);
}

int VideoPlayer::getFrameHeight() {
	return frameHeight;
}

int VideoPlayer::getFrameWidth() {
	return frameWidth;
}

VideoPlayer::~VideoPlayer()
{
	FILE* fp = fopen("dump_first_frame12.dat", "wb");
	fwrite("TST", 3, 1, fp);
	fclose(fp);

	//Free all stuff
	if (imageRt)
		imageRt->drop();
	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	// avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);
}
