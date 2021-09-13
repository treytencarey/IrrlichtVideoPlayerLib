#include "ScreenRecorder.h"

using namespace std;

/* initialize the resources*/
ScreenRecorder::ScreenRecorder()
{

	avdevice_register_all();
	cout << "\nall required functions are registered successfully";
}

/* uninitialize the resources */
ScreenRecorder::~ScreenRecorder()
{

	avformat_close_input(&pAVFormatContext);
	if (!pAVFormatContext)
	{
		cout << "\nfile closed sucessfully";
	}
	else
	{
		cout << "\nunable to close the file";
		exit(1);
	}

	avformat_free_context(pAVFormatContext);
	if (!pAVFormatContext)
	{
		cout << "\navformat free successfully";
	}
	else
	{
		cout << "\nunable to free avformat context";
		exit(1);
	}

}

/* function to capture and store data in frames by allocating required memory and auto deallocating the memory.   */
int ScreenRecorder::CaptureVideoFrames()
{
	int flag;
	int frameFinished;//when you decode a single packet, you still don't have information enough to have a frame [depending on the type of codec, some of them //you do], when you decode a GROUP of packets that represents a frame, then you have a picture! that's why frameFinished will let //you know you decoded enough to have a frame.

	int frame_index = 0;
	value = 0;

	pAVPacket = av_packet_alloc();

	pAVFrame = av_frame_alloc();
	if (!pAVFrame)
	{
		cout << "\nunable to release the avframe resources";
		exit(1);
	}

	outFrame = av_frame_alloc();//Allocate an AVFrame and set its fields to default values.
	if (!outFrame)
	{
		cout << "\nunable to release the avframe resources for outframe";
		exit(1);
	}

	int width = outAVCodecContext->width;
	int height = outAVCodecContext->height;
	int   numBytes;
	unsigned char* buffer = NULL;
	// Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	// Setup the data pointers and linesizes based on the specified image parameters and the provided array.
	value = av_image_fill_arrays(outFrame->data, outFrame->linesize, buffer, AV_PIX_FMT_RGB24, width, height, 1); // returns : the size in bytes required for src
	if (value < 0)
	{
		cout << "\nerror in filling image array";
	}

	SwsContext* swsCtx_;

	// Allocate and return swsContext.
	// a pointer to an allocated context, or NULL in case of error
	// Deprecated : Use sws_getCachedContext() instead.
	swsCtx_ = sws_getContext(pAVCodecContext->width,
		pAVCodecContext->height,
		AV_PIX_FMT_YUV420P,
		width, height,
		AV_PIX_FMT_RGB24,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!swsCtx_)
	{
		cout << "\nerror getting context";
		exit(1);
	}

	avcodec_flush_buffers(pAVCodecContext);

	int ii = 0;
	int no_frames = 100;

	AVPacket outPacket;
	int j = 0;

	int got_picture;

	while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
	{
		if (ii++ == no_frames)break;
		if (pAVPacket->stream_index == VideoStreamIndx)
		{
			avcodec_send_packet(pAVCodecContext, pAVPacket);
			int ret1 = 0;
			while (ret1 >= 0)
			{
				ret1 = avcodec_receive_frame(pAVCodecContext, pAVFrame);
				if (ret1 == AVERROR(EAGAIN) || ret1 == AVERROR_EOF)
				{
					ret1 = 0;
					avcodec_send_packet(pAVCodecContext, pAVPacket);
					continue;
				}
				if (ret1 < 0)
					break;

				sws_scale(swsCtx_, (uint8_t const* const*)&pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);
				outPacket = *av_packet_alloc();
				outPacket.data = NULL;    // packet data will be allocated by the encoder
				outPacket.size = 0;

				avcodec_send_frame(outAVCodecContext, outFrame);
				int ret2 = 0;
				while (ret2 >= 0)
				{
					ret2 = avcodec_receive_packet(outAVCodecContext, &outPacket);
					if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF)
					{
						ret2 = 0;
						avcodec_send_frame(outAVCodecContext, outFrame);
						continue;
					}
					if (ret2 < 0)
						break;

					if (outPacket.pts != AV_NOPTS_VALUE)
						outPacket.pts = av_rescale_q(outPacket.pts, video_st->time_base, video_st->time_base);
					if (outPacket.dts != AV_NOPTS_VALUE)
						outPacket.dts = av_rescale_q(outPacket.dts, video_st->time_base, video_st->time_base);

					printf("Write frame %3d (size= %2d)\n", j++, outPacket.size / 1000);
					if (av_write_frame(outAVFormatContext, &outPacket) != 0)
					{
						cout << "\nerror in writing video frame";
					}

					av_packet_unref(&outPacket);
				} // got_picture

				av_packet_unref(&outPacket);
			} // frameFinished

		}
	}// End of while-loop

	value = av_write_trailer(outAVFormatContext);
	if (value < 0)
	{
		cout << "\nerror in writing av trailer";
		exit(1);
	}


	//THIS WAS ADDED LATER
	av_free(buffer);

}

/* establishing the connection between camera or screen through its respective folder */
int ScreenRecorder::openCamera()
{

	value = 0;
	options = NULL;
	pAVFormatContext = NULL;

	pAVFormatContext = avformat_alloc_context();//Allocate an AVFormatContext.
/*

X11 video input device.
To enable this input device during configuration you need libxcb installed on your system. It will be automatically detected during configuration.
This device allows one to capture a region of an X11 display.
refer : https://www.ffmpeg.org/ffmpeg-devices.html#x11grab
*/
/* current below is for screen recording. to connect with camera use v4l2 as a input parameter for av_find_input_format */
	pAVInputFormat = (AVInputFormat*)av_find_input_format("gdigrab");
	value = avformat_open_input(&pAVFormatContext, "desktop", pAVInputFormat, NULL);
	if (value != 0)
	{
		cout << "\nerror in opening input device";
		exit(1);
	}

	/* set frame per second */
	value = av_dict_set(&options, "framerate", "30", 0);
	if (value < 0)
	{
		cout << "\nerror in setting dictionary value";
		exit(1);
	}

	value = av_dict_set(&options, "preset", "medium", 0);
	if (value < 0)
	{
		cout << "\nerror in setting preset values";
		exit(1);
	}

	//	value = avformat_find_stream_info(pAVFormatContext,NULL);
	if (value < 0)
	{
		cout << "\nunable to find the stream information";
		exit(1);
	}

	VideoStreamIndx = -1;

	// Retrieve stream information
	if (avformat_find_stream_info(pAVFormatContext, NULL) < 0)
		return -1; // Couldn't find stream information

	/* find the first video stream index . Also there is an API available to do the below operations */
	for (int i = 0; i < pAVFormatContext->nb_streams; i++) // find video stream posistion/index.
	{
		if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VideoStreamIndx = i;
			break;
		}

	}

	if (VideoStreamIndx == -1)
	{
		cout << "\nunable to find the video stream index. (-1)";
		exit(1);
	}

	pAVCodec = (AVCodec*)avcodec_find_decoder(pAVFormatContext->streams[VideoStreamIndx]->codecpar->codec_id);
	if (pAVCodec == NULL)
	{
		cout << "\nunable to find the decoder";
		exit(1);
	}
	pAVCodecContext = avcodec_alloc_context3(pAVCodec);

	avcodec_parameters_to_context(pAVCodecContext, pAVFormatContext->streams[VideoStreamIndx]->codecpar);
	if (pAVCodecContext->codec_id == AV_CODEC_ID_H264) {
		av_opt_set(pAVCodecContext, "preset", "ultrafast", 0);
	}
	else if (pAVCodecContext->codec_id == AV_CODEC_ID_H265) {
		av_opt_set(pAVCodecContext, "preset", "ultrafast", 0);
	}
	else
	{
		av_opt_set_int(pAVCodecContext, "lossless", 1, 0);
	}

	value = avcodec_open2(pAVCodecContext, pAVCodec, NULL);//Initialize the AVCodecContext to use the given AVCodec.
	if (value < 0)
	{
		cout << "\nunable to open the av codec";
		exit(1);
	}
}

/* initialize the video output file and its properties  */
int ScreenRecorder::init_outputfile()
{
	outAVFormatContext = NULL;
	value = 0;
	output_file = "output.h264";

	avformat_alloc_output_context2(&outAVFormatContext, NULL, NULL, output_file);
	if (!outAVFormatContext)
	{
		cout << "\nerror in allocating av format output context";
		exit(1);
	}

	/* Returns the output format in the list of registered output formats which best matches the provided parameters, or returns NULL if there is no match. */
	output_format = (AVOutputFormat*)av_guess_format(NULL, output_file, NULL);
	if (!output_format)
	{
		cout << "\nerror in guessing the video format. try with correct format";
		exit(1);
	}

	video_st = avformat_new_stream(outAVFormatContext, NULL);
	if (!video_st)
	{
		cout << "\nerror in creating a av format new stream";
		exit(1);
	}

	outAVCodecContext = avcodec_alloc_context3(outAVCodec);
	if (!outAVCodecContext)
	{
		cout << "\nerror in allocating the codec contexts";
		exit(1);
	}

	outAVCodec = (AVCodec*)avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!outAVCodec)
	{
		cout << "\nerror in finding the av codecs. try again with correct codec";
		exit(1);
	}

	/* set property of the video file */
	outAVCodecContext = avcodec_alloc_context3(outAVCodec);
	outAVCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	outAVCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	outAVCodecContext->bit_rate = 400000; // 2500000
	outAVCodecContext->width = 4480;
	outAVCodecContext->height = 1440;
	outAVCodecContext->gop_size = 3;
	outAVCodecContext->max_b_frames = 2;
	outAVCodecContext->time_base.num = 1;
	outAVCodecContext->time_base.den = 30; // 15fps

	if (codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(outAVCodecContext->priv_data, "preset", "slow", 0);
	}

	/* Some container formats (like MP4) require global headers to be present
	   Mark the encoder so that it behaves accordingly. */

	if (outAVFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		outAVCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	value = avcodec_open2(outAVCodecContext, outAVCodec, NULL);
	if (value < 0)
	{
		cout << "\nerror in opening the avcodec";
		exit(1);
	}

	/* create empty video file */
	if (!(outAVFormatContext->flags & AVFMT_NOFILE))
	{
		if (avio_open2(&outAVFormatContext->pb, output_file, AVIO_FLAG_WRITE, NULL, NULL) < 0)
		{
			cout << "\nerror in creating the video file";
			exit(1);
		}
	}

	if (!outAVFormatContext->nb_streams)
	{
		cout << "\noutput file dose not contain any stream";
		exit(1);
	}

	/* imp: mp4 container or some advanced container file required header information*/
	//value = avformat_write_header(outAVFormatContext, &options);
	if (value < 0)
	{
		cout << "\nerror in writing the header context";
		exit(1);
	}

	/*
	// uncomment here to view the complete video file informations
	cout<<"\n\nOutput file information :\n\n";
	av_dump_format(outAVFormatContext , 0 ,output_file ,1);
	*/
}

