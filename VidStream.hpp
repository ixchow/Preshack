#ifndef VIDSTREAM_HPP
#define VIDSTREAM_HPP

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
extern "C" {
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <string>
#include <deque>

using std::deque;
using std::string;

//most of this stuff is taken from
// http://www.cse.msu.edu/~minutsil/linux/ffmpeg_decode.C
//
//(I've added the wrapper object so I can easily use several at once.)
//
//Audio stuff based on ffplay.c
//
//Updated based on http://web.me.com/dhoerl/Home/Tech_Blog/Entries/2009/1/22_Revised_avcodec_sample.c.html

//Updated again based on 'ffmpeg/doc/examples/demuxing_decoding.c' from the ffmpeg source

class VidStream {
public:
	VidStream(std::string const &filename);
	VidStream(VidStream const &other); //undefined for a reason.
	~VidStream();
	
	//move video forward, or return 'false' if at end of video.
	bool advance();

	double timestamp = 0.0;
	uint32_t width = 0, height = 0; //width and height of current video frame

	bool done = false;

	//internals:

	AVFrame *frame = NULL;
	AVFormatContext *fcx = NULL;
	AVCodecContext *video_ctx = NULL;
	uint32_t video_index = -1U;

	void decode(); //helper!

	SwsContext *sws = NULL;
	AVPixelFormat sws_format = AV_PIX_FMT_NONE;

	uint8_t *rgb_data[4] = {NULL, NULL, NULL, NULL};
	int rgb_linesizes[4] = {0, 0, 0, 0};

	double video_timebase = 0.0;

	
	/*
	AVFrame *frame = NULL;
	AVFrame *frameRGB = NULL;
	uint8_t *frame_buffer = NULL;
	std::vector< uint8_t > video_pixels;

	SwsContext *sws_context = NULL;
	
	unsigned int video_index = -1U; //index 'o video stream
	unsigned int audio_index = -1U; //index 'o audio stream

	std::vector< uint8_t > audio_samples;

	deque< AVPacket * > video_queue;
	deque< AVPacket * > audio_queue;

	bool fresh_audio_packet;
	int audio_packet_so_far;

	bool is_open;
	
	static void avcodec_init();
	*/
};

#endif
