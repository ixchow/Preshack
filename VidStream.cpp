#include "VidStream.hpp"
#include <iostream>
#include <assert.h>

using std::cout;
using std::endl;

VidStream::VidStream() {
	video_clock = audio_clock = 0.0;
	fcx = NULL;
	video_codec = NULL;
	audio_codec = NULL;
	frame = NULL;
	frameRGB = NULL;
	frame_buffer = NULL;

	sws_context = NULL;

	video_index = audio_index = 0;

	VidStream::avcodec_init();

	frame = avcodec_alloc_frame();

	fresh_audio_packet = true;
	audio_packet_so_far = 0;
	
	is_open = false;
}

bool VidStream::open(string const &filename) {
	
	if (is_open) close();
	
	fcx = NULL;
	video_codec = audio_codec = NULL;
	fresh_audio_packet = true;
	audio_packet_so_far = 0;
	video_clock = audio_clock = 0.0;
	
	int err = 0;
	err = av_open_input_file(&fcx, filename.c_str(), NULL, 0, NULL);
	if (err < 0) {
		cout << "Error " << err << " opening " << filename << "." << endl;
		fcx = NULL;
		return false;
	}

	err = av_find_stream_info(fcx);
	//check error?
	if (err < 0) {
		cout << "Error " << err << " getting stream info." << endl;
	}
	
	for (unsigned int index = 0; index < fcx->nb_streams; ++index) {
		switch (fcx->streams[index]->codec->codec_type) {
			case CODEC_TYPE_VIDEO:
				video_index = index;
				video_codec = fcx->streams[index]->codec;
				break;
			case CODEC_TYPE_AUDIO:
				audio_index = index;
				audio_codec = fcx->streams[index]->codec;
				break;
			default:
				break;
		}
	}

	if (video_codec == NULL) {
		cout << "Couldn't find a video stream." << endl;
	}
	if (audio_codec == NULL) {
		cout << "Couldn't find an audio stream (but we'll keep rolling right along)." << endl;
	}

	if (!video_codec && !audio_codec) {
		cout << "No video or audio. Bailing." << endl;
		close();
		return false;
	}

	if (video_codec) { //this opens video codec
		AVCodec *codec = avcodec_find_decoder(video_codec->codec_id);
		if (!codec) {
			cout << "Couldn't find a decoder for video" << endl;
			close();
			return false;
		}

		err = avcodec_open(video_codec, codec);
		if (err < 0) {
			cout << "Couldn't open video codec" << endl;
			close();
			return false;
		}
	}

	if (audio_codec) { //this opens audio codec
		AVCodec *codec = avcodec_find_decoder(audio_codec->codec_id);
		if (!codec) {
			cout << "Couldn't find a decoder for audio" << endl;
			close();
			return false;
		}

		err = avcodec_open(audio_codec, codec);
		if (err < 0) {
			cout << "Couldn't open audio codec" << endl;
			close();
			return false;
		}
	}


	if (video_codec) {
		frame=avcodec_alloc_frame();
		frameRGB=avcodec_alloc_frame();
		assert(frame);
		assert(frameRGB);

		unsigned int bytes = avpicture_get_size(PIX_FMT_RGB24, video_codec->width, video_codec->height);
		frame_buffer = new uint8_t[bytes];
		avpicture_fill((AVPicture *)frameRGB, frame_buffer, PIX_FMT_RGB24, video_codec->width, video_codec->height);

		//avpicture_alloc(&pict, PIX_FMT_RGB24, video_codec->width, video_codec->height);

		w = video_codec->width;
		h = video_codec->height;

		sws_context = sws_getCachedContext(NULL,w,h,video_codec->pix_fmt, w,h,PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

		cout << "Video time-base for '" << filename << "' is " << video_codec->time_base.num << " / " << video_codec->time_base.den << endl;

	}

	is_open = true;
	return true;
}

void VidStream::close() {
	if (fcx) {
		av_close_input_file(fcx);
	}
	if (video_codec) {
		//avpicture_free(&pict);
	}
	while (!video_queue.empty()) {
		av_free_packet(video_queue.front());
		delete video_queue.front();
		video_queue.pop_front();
	}
	while (!audio_queue.empty()) {
		av_free_packet(audio_queue.front());
		delete audio_queue.front();
		audio_queue.pop_front();
	}
	if (frame) {
		av_free(frame);
		frame = NULL;
	}
	if (frameRGB) {
		av_free(frameRGB);
		frameRGB = NULL;
	}
	if (frame_buffer) {
		delete[] frame_buffer;
		frame_buffer = NULL;
	}
	is_open = false;

}

VidStream::~VidStream() {
	if (is_open) close();

	av_free(frame);
}

const unsigned char *VidStream::get_video_pixels() {
	assert(frameRGB);
	return frameRGB->data[0];
}

const unsigned char *VidStream::get_audio_samples() {
	return audio_frame;
}

bool VidStream::advance_audio() {
	if (!audio_codec) return false;
	while (1) {
		//fetch packets.
		while (audio_queue.empty() && advance()) /* spin */ ;
		if (audio_queue.empty()) return false;


		if (fresh_audio_packet) {
			if ((unsigned long long)audio_queue.front()->pts != AV_NOPTS_VALUE) {
				audio_clock = (double)audio_queue.front()->pts / AV_TIME_BASE;
				audio_len = 0;
			}
			fresh_audio_packet = false;
		}
		
		//if the packet isn't empty:
		if (audio_queue.front()->size != audio_packet_so_far) {
			//decode frames
			int data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
			audio_queue.front()->data += audio_packet_so_far;
			audio_queue.front()->size -= audio_packet_so_far;
			int used = avcodec_decode_audio3(audio_codec, (int16_t *)audio_frame, &data_size, audio_queue.front());
			audio_queue.front()->data -= audio_packet_so_far;
			audio_queue.front()->size += audio_packet_so_far;

			// < 0 means something wrong with this packet, so drop it.
			if (used < 0) {
				audio_packet_so_far = audio_queue.front()->size;
			} else {
		
				audio_packet_so_far += used;

				//if this frame actually contained data...
				if (data_size > 0) {
					//update clock from length of last frame.
					audio_clock += (double)audio_len / (audio_codec->sample_rate * 2 * audio_codec->channels);

					//store length of frame so program will know.
					audio_len = data_size;

					return true;
				}
			}
		}

		//dispose packets that are finished with.
		while (!audio_queue.empty() && audio_queue.front()->size == audio_packet_so_far) {
			av_free_packet(audio_queue.front());
			delete audio_queue.front();
			audio_queue.pop_front();
			fresh_audio_packet = true;
			audio_packet_so_far = 0;
		}

	}
}

bool VidStream::advance_video() {
	if (!video_codec) return false;
	while (1) {
		while (video_queue.empty() && advance()) /* find a new video packet */ ;
		if (video_queue.empty()) return false;
		AVPacket *pkt = video_queue.front();
		video_queue.pop_front();

		int len1, got_picture;
		len1 = avcodec_decode_video2(video_codec, frame, &got_picture, pkt);
		if (got_picture) {
			if (frame->pts != (int64_t)AV_NOPTS_VALUE && frame->pts != 0) {
				video_clock = (double)(uint64_t)frame->pts * video_codec->time_base.num / (double)video_codec->time_base.den;
			} else {
				if (video_codec->time_base.num == 1 && video_codec->time_base.den == 60) {
					static bool warned = false;
					if (!warned) {
						cout << "****WARNING****: playing 60fps at 30fps." << endl;
						warned = true;
					}
					video_clock += (double)1 / (double)30;
				} else if (video_codec->time_base.num == 1 && video_codec->time_base.den == 50) {
					static bool warned = false;
					if (!warned) {
						cout << "****WARNING****: playing 50fps at 25fps." << endl;
						warned = true;
					}
					video_clock += (double)1 / (double)25;
				} else {
					video_clock += (double)video_codec->time_base.num / (double)video_codec->time_base.den;
				}
			}

			assert(video_codec->width == w && video_codec->height == h);
			sws_scale(sws_context, frame->data, frame->linesize, 0, video_codec->height, frameRGB->data, frameRGB->linesize);

			av_free_packet(pkt);
			delete pkt;
			
			return true;
		}
		av_free_packet(pkt);
		delete pkt;
	}
	return false;
}

void VidStream::discard_audio() {
	while (!audio_queue.empty()) {
		av_free_packet(audio_queue.front());
		audio_queue.pop_front();
	}
}

void VidStream::discard_video() {
	while (!video_queue.empty()) {
		av_free_packet(video_queue.front());
		video_queue.pop_front();
	}
}

int VidStream::audio_rate() const {
	if (audio_codec) return audio_codec->sample_rate;
	return 0;
}

int VidStream::audio_channels() const {
	if (audio_codec) return audio_codec->channels;
	return 0;
}

bool VidStream::has_audio() const {
	return audio_codec != NULL;
}

bool VidStream::has_video() const {
	return video_codec != NULL;
}

double VidStream::audio_time() const {
	return audio_clock;
}

double VidStream::video_time() const {
	return video_clock;
}

bool VidStream::advance() {
	AVPacket *pkt = new AVPacket;
	if (av_read_frame(fcx, pkt) >= 0) {
		if (pkt->stream_index == (signed)video_index && video_codec) {
		//	int len1, got_picture;
		//	len1 = avcodec_decode_video(video_codec, frame, &got_picture, pkt.data, pkt.size);
		//	if (got_picture) {
		//		new_picture = true;
		//	}
			//This really allocates memory for the pkt if it wasn't already allocated:
			av_dup_packet(pkt);
			video_queue.push_back(pkt);
		} else if (pkt->stream_index == (signed)audio_index && audio_codec) {
			av_dup_packet(pkt);
			audio_queue.push_back(pkt);
		} else {
			av_free_packet(pkt);
			delete pkt;
		}
		return true;
	}
	return false;
}


void VidStream::avcodec_init() {
	static int refcount = 0;
	if (refcount == 0) {
		av_register_all();
	}
	++refcount;
};
