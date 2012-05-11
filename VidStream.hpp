#ifndef VIDSTREAM_HPP
#define VIDSTREAM_HPP

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
extern "C" {
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
//(I've added the wrapper object so I can easilly use several at once.)
//
//Audio stuff based on ffplay.c
//
//Updated based on http://web.me.com/dhoerl/Home/Tech_Blog/Entries/2009/1/22_Revised_avcodec_sample.c.html

class VidStream {
public:
	VidStream();
	VidStream(VidStream const &other); //undefined for a reason.
	~VidStream();
	
	bool open(string const &filename);
	void close();

	const unsigned char *get_video_pixels();
	const unsigned char *get_audio_samples();

	bool advance_audio();
	bool advance_video();

	//clears the respective packet queues:
	void discard_audio();
	void discard_video();

	int w,h; //width and height of current video frame
	int audio_len; //length of current audio frame

	//audio is always 16-bit (in system order), methinks.
	int audio_rate() const;
	int audio_channels() const;

	bool has_audio() const;
	bool has_video() const;

	double video_time() const;
	double audio_time() const;

private:
	bool advance();

	//time at the start of the present audio/video frames:
	double video_clock;
	double audio_clock;
	
	AVFormatContext *fcx;
	AVCodecContext *video_codec;
	AVCodecContext *audio_codec;
	
	AVFrame *frame;
	AVFrame *frameRGB;
	uint8_t *frame_buffer;

	SwsContext *sws_context;
	
	unsigned int video_index; //index 'o video stream
	unsigned int audio_index; //index 'o audio stream

	unsigned char audio_frame[AVCODEC_MAX_AUDIO_FRAME_SIZE];

	deque< AVPacket * > video_queue;
	deque< AVPacket * > audio_queue;

	bool fresh_audio_packet;
	int audio_packet_so_far;

	bool is_open;
	
	static void avcodec_init();
};

#endif
