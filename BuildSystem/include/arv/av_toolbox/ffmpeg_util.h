//
// Created by jerett on 16/12/2.
//

#ifndef INSMEDIAUTIL_FFMPEG_OBJ_DEFINE_H
#define INSMEDIAUTIL_FFMPEG_OBJ_DEFINE_H

extern "C" { 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

#include "platform.h"
#if (__APPLE__)
#include <VideoToolbox/VideoToolbox.h>
#endif
#include "sp.h"
#include <string>

struct AVPacket;
struct AVFrame;
struct AVCodec;
struct AVCodecContext;
struct AVCodecParameters;
struct AVFormatContext;
struct AVRational;
struct AVStream;

namespace ins {

struct ARVPacket: public AVPacket {
  AVMediaType media_type;
};

///call many times will ensure init only once
void InitFFmpeg();

std::string FFmpegErrorToString(int code);

//sp<AVPacket> NewAVPacket();
sp<ARVPacket> NewPacket();
sp<ARVPacket> NewH264Packet(const uint8_t *data, int size, int64_t pts, int64_t dts, bool keyframe, int stream_index);
sp<ARVPacket> NewAACPacket(const uint8_t *data, int size, int64_t pts, int stream_index);

sp<AVFrame> NewAVFrame();
sp<AVFrame> NewAVFrame(int width, int height, AVPixelFormat fmt);
sp<AVFrame> CloneAVFrame(const sp<AVFrame> &src);

sp<AVCodecContext> NewAVCodecContext(AVCodec*);

sp<AVCodecParameters> NewAVCodecParameters();

sp<AVCodecContext> NewAVDecodeContextFromID(AVCodecID id);
sp<AVCodecContext> NewAVDecodeContextFromName(const std::string &name);
sp<AVCodecContext> NewAVHwaccelDecodeContextFromID(AVCodecID id);

sp<AVCodecContext> NewAVEncodeContextFromID(AVCodecID id);
sp<AVCodecContext> NewAVHwaccelEncodeContextFromID(AVCodecID id);
sp<AVCodecContext> NewAVEncodeContextFromName(const std::string &name);

sp<AVFormatContext> NewOutputAVFormatContext(const std::string &format_name,
                                             const std::string &filename);

sp<AVStream> NewAVStream();
sp<AVStream> AddAVStream(const sp<AVFormatContext> &sp_fmt);
sp<AVStream> NewVideoStream(int width, int height,
                            AVRational framerate, AVRational timebase,
                            AVCodecID codec_id, AVPixelFormat pixel_fmt);
sp<AVStream> NewH264Stream(int width, int height,
                           AVRational framerate, AVRational timebase,
                           AVCodecID codec_id, AVPixelFormat pixel_fmt,
                           const uint8_t *extradata, int extradata_size);
sp<AVStream> NewAACStream(AVSampleFormat sample_fmt,
                          uint64_t channel_layout, int channels, int sample_rate,
                          uint8_t *extradata, int extradata_size,
                          AVRational timebase);

#if (__APPLE__)
/// copy CVPixelBuffer to corresponding frame,  will use memcpy
/// @param apple pixel 
/// @return frame
sp<AVFrame> CopyCVPixelBufferToFrame(CVPixelBufferRef pixel);

/// ref CVPixelBuffer to corresponding frame,  don't use memcpy
/// @param pixel pixel buffer
/// @return frame
sp<AVFrame> RefCVPixelBufferToFrame(CVPixelBufferRef pixel);
#endif

inline bool IsKeyFrame(const sp<AVPacket> &pkt) {
  return (pkt->flags & AV_PKT_FLAG_KEY) > 0;
}

inline int64_t TimestampToMs(int64_t pts, const AVRational &r) {
  return av_rescale_q(pts, r, {1, 1000});
}

inline int64_t MsToTimestamp(int64_t ms, const AVRational &r) {
  return av_rescale_q(ms, {1, 1000}, r);
}

}

#endif //INSMEDIAUTIL_FFMPEG_OBJ_DEFINE_H
