#include "VidStream.hpp"
#include <iostream>
#include <stdexcept>
#include <assert.h>

using std::cout;
using std::endl;

VidStream::VidStream(std::string const &filename) {
	{ //init avcodec once only:
		static bool inited = false;
		if (!inited) {
			av_register_all();
			inited = true;
		}
	}

	std::string err_prefix = "[" + filename + "] ";

	int err = 0;
	err = avformat_open_input(&fcx, filename.c_str(), NULL, NULL);
	if (err < 0) {
		throw std::runtime_error(err_prefix + "Error " + std::to_string(err) + " opening file.");
	}

	err = avformat_find_stream_info(fcx, NULL);
	if (err < 0) {
		throw std::runtime_error(err_prefix + "Error " + std::to_string(err) + " getting stream info.");
	}

	video_index = av_find_best_stream(fcx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if ((int)video_index < 0) {
		throw std::runtime_error(err_prefix + "Error " + std::to_string(err) + " finding video stream index.");
	}

	AVStream *stream = fcx->streams[video_index];

	video_timebase = av_q2d(stream->time_base);

	AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!dec) {
		throw std::runtime_error(err_prefix + "Failed to find decoder.");
	}

	video_ctx = avcodec_alloc_context3(dec);
	if (!video_ctx) {
		throw std::runtime_error(err_prefix + "Failed to allocate decode context.");
	}

	err = avcodec_parameters_to_context(video_ctx, stream->codecpar);
	if (err < 0) {
		throw std::runtime_error(err_prefix + "Error " + std::to_string(err) + " copying parameters to codec context.");
	}

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "refcounted_frames", "1", 0);
	err = avcodec_open2(video_ctx, dec, &opts);
	av_dict_free(&opts);

	if (err < 0) {
		throw std::runtime_error(err_prefix + "Error " + std::to_string(err) + " opening codec.");
	}

	//... these can change ...
	width = video_ctx->width;
	height = video_ctx->height;

	frame = av_frame_alloc();
}

VidStream::~VidStream() {
	if (frame) {
		av_frame_free(&frame);
		frame = NULL;
	}
	if (rgb_data[0]) {
		av_freep(&rgb_data[0]);
		rgb_data[0] = NULL;
	}
	if (sws) {
		sws_freeContext(sws);
		sws = NULL;
	}
	if (video_ctx) {
		avcodec_free_context(&video_ctx);
		video_ctx = NULL;
	}
	if (fcx) {
		avformat_close_input(&fcx);
		fcx = NULL;
	}
}

void VidStream::decode() {
	//get a new sws context if needed:
	if (sws == NULL || (int)width != frame->width || (int)height != frame->height || frame->format != sws_format) {
		if (sws) {
			sws_freeContext(sws);
			sws = NULL;
		}
		if (rgb_data[0]) {
			av_freep(&rgb_data[0]);
			rgb_data[0] = NULL;
		}
		width = frame->width;
		height = frame->height;
		sws_format = (AVPixelFormat)frame->format;

		sws = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

		if (!sws) {
			std::cerr << "Failed to build sws context to convert frame." << std::endl;
			return;
		}

		int err = av_image_alloc(rgb_data, rgb_linesizes, width, height, AV_PIX_FMT_RGB24, 1);
		if (err < 0) {
			std::cerr << "Failed to allocate decode target image." << std::endl;
			return;
		}
	}

	int h = sws_scale(sws, frame->data, frame->linesize, 0, frame->height, rgb_data, rgb_linesizes);
	if (h != (int)height) {
		std::cerr << "Warning: scaling did something strange with height." << std::endl;
	}

	int64_t ts = av_frame_get_best_effort_timestamp(frame);
	timestamp = ts * video_timebase;
	//std::cout << "Timestamp: " << timestamp << "." << std::endl; //DEBUG
}


bool VidStream::advance() {
	if (done) return false;

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	while (true) {
		{ //try to read a frame:
			int err = avcodec_receive_frame(video_ctx, frame);
			if (err == 0) {
				//got a frame, decode and store somewhere.
				decode();
				av_frame_unref(frame);
				return true;
			} else {
				if (err == AVERROR_EOF) {
					done = true;
					return false; //codec fully drained.
				} else if (err == AVERROR(EAGAIN)) {
					//needs more data, fall through.
				} else {
					std::cerr << "Error " << err << " decoding frames." << std::endl;
					return false;
				}
			}
		}
		{ //couldn't read a frame, so give more data to codec:
		
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;

			int err = av_read_frame(fcx, &pkt);
			if (err == 0) {
				if (pkt.stream_index == (int)video_index) {
					err = avcodec_send_packet(video_ctx, &pkt);
					av_packet_unref(&pkt);
					if (err != 0) {
						std::cerr << "Error " << err << " trying to send packet." << std::endl;
						return false;
					}
				} else {
					av_packet_unref(&pkt);
				}
			} else {
				avcodec_send_packet(video_ctx, NULL);
				if (err == AVERROR_EOF) {
					std::cerr << "Flush packet returned AVERROR_EOF, which I'm not sure is proper but might be." << std::endl; //DEBUG
				} else if (err != 0) {
					std::cerr << "Error " << err << " trying to send flush command." << std::endl;
					return false;
				}
			}
		}
	}
}
