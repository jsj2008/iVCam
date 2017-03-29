//
// Created by jerett on 16/12/2.
//

#include "ffmpeg_util.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#if (__APPLE__)
#include <libavcodec/videotoolbox.h>
#endif
}
#include <mutex>
#include <llog/llog.h>

#if __APPLE__
static enum AVPixelFormat UseAVPixlFmtVideoToolbox(AVCodecContext *s, const enum AVPixelFormat *pix_fmts) {
  auto ret = av_videotoolbox_default_init(s);
  if (ret < 0) {
    LOG(ERROR) << "videotoolbox init err:" << ins::FFmpegErrorToString(ret);
    LOG(VERBOSE) << "use h264 decode.";
    return AV_PIX_FMT_YUV420P;
  } else {
    LOG(VERBOSE) << "use videotoolbox decode.";
    return AV_PIX_FMT_VIDEOTOOLBOX;
  }
}
#endif // __APPLE__


namespace ins {

//static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
//  if (level <= AV_LOG_VERBOSE) {
//    vprintf(fmt, vl);
//  }
//}

void InitFFmpeg() {
  static std::once_flag init_once;
  std::call_once(init_once, []() {
    LOG(INFO) << "ffmpeg init";
    av_register_all();
    avformat_network_init();
//    av_log_set_level(AV_LOG_ERROR);
//    av_log_set_callback(av_log_callback);
  });
}

sp<ARVPacket> NewPacket() {
//  AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
  auto packet = static_cast<ARVPacket*>(malloc(sizeof(ARVPacket)));
  CHECK(packet != nullptr) << "malloc AVPacket return nullptr";
  av_init_packet(packet);
  packet->media_type = AVMEDIA_TYPE_UNKNOWN;
  sp<ARVPacket> sp_pkt(packet, [](AVPacket *pkt){
    av_packet_unref(pkt); 
    free(pkt);
  });
  return sp_pkt;
}

sp<AVFrame> NewAVFrame() {
  auto frame = av_frame_alloc();
  CHECK(frame != nullptr) << "alloc frame nullptr";
  sp<AVFrame> sp_frame(frame, [](AVFrame *f) {
    av_frame_free(&f);
  });
  return sp_frame;
}

sp<AVFrame> NewAVFrame(int width, int height, AVPixelFormat fmt) {
  AVFrame *frame = av_frame_alloc();
  frame->width = width;
  frame->height = height;
  frame->format = fmt;
  av_image_alloc(frame->data, frame->linesize, width, height, fmt, 1);
  CHECK(frame != nullptr) << "alloc frame nullptr";
  sp<AVFrame> sp_frame(frame, [](AVFrame *f) {
    av_freep(&f->data[0]);
    av_frame_free(&f);
  });
  return sp_frame;
}

sp<AVFrame> CloneAVFrame(const sp<AVFrame> &src) {
  auto dst_frame = av_frame_clone(src.get());
  CHECK(dst_frame != nullptr);
  sp<AVFrame> sp_frame(dst_frame, [](AVFrame *frame) {
    av_frame_free(&frame);
  });
  return sp_frame;
}

sp<ARVPacket> NewH264Packet(const uint8_t * data, int size, int64_t pts, int64_t dts,
                           bool keyframe, int stream_index) {
  auto pkt = NewPacket();
  pkt->media_type = AVMEDIA_TYPE_VIDEO;
  CHECK(pkt != nullptr);
  pkt->data = new uint8_t[size];
  pkt->size = size;
  memcpy(pkt->data, data, size);
  pkt->pts = pts;
  pkt->dts = dts;
  pkt->flags = keyframe ? AV_PKT_FLAG_KEY : 0;
  pkt->stream_index = stream_index;
  return pkt;
}

sp<ARVPacket> NewAACPacket(const uint8_t * data, int size, int64_t pts, int stream_index) {
  auto pkt = NewPacket();
  pkt->media_type = AVMEDIA_TYPE_AUDIO;
  CHECK(pkt != nullptr);
  pkt->data = new uint8_t[size];
  pkt->size = size;
  memcpy(pkt->data, data, size);
  pkt->pts = pts;
  pkt->dts = pts;
  pkt->flags = AV_PKT_FLAG_KEY;
  pkt->stream_index = stream_index;
  return pkt;
}

sp<AVCodecParameters> NewAVCodecParameters() {
  auto par = avcodec_parameters_alloc();
  CHECK(par != nullptr);
  sp<AVCodecParameters> sp_cp(par, [](AVCodecParameters *p) {
    avcodec_parameters_free(&p);
  });
  return sp_cp;
}

sp<AVStream> NewVideoStream(int width, int height, 
                            AVRational framerate, AVRational timebase, 
                            AVCodecID codec_id, AVPixelFormat pixel_fmt) {
  auto vst = NewAVStream();
  vst = NewAVStream();
  vst->codecpar->width = width;
  vst->codecpar->height = height;
  vst->codecpar->format = pixel_fmt;
  vst->codecpar->codec_id = codec_id;
  vst->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  vst->codecpar->codec_tag = 0;
  vst->avg_frame_rate = framerate;
  vst->time_base = timebase;
  return vst;
}



sp<AVStream> NewH264Stream(int width, int height,
                           AVRational framerate, AVRational timebase,
                           AVCodecID codec_id, AVPixelFormat pixel_fmt,
                           const uint8_t *extradata, int extradata_size) {
  auto vst = NewVideoStream(width, height, framerate, timebase, codec_id, pixel_fmt);
  if (!vst) return nullptr;
  vst->codecpar->extradata = reinterpret_cast<uint8_t*>(av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
  vst->codecpar->extradata_size = extradata_size;
  memcpy(vst->codecpar->extradata, extradata, extradata_size);
  return vst;
}

sp<AVStream> NewAACStream(AVSampleFormat sample_fmt, 
                          uint64_t channel_layout, int channels, int sample_rate, 
                          uint8_t *extradata, int extradata_size, 
                          AVRational timebase) {
  auto aac_stream = NewAVStream();
  aac_stream->codecpar->format = sample_fmt;
  aac_stream->codecpar->channel_layout = channel_layout;
  aac_stream->codecpar->channels = channels;
  aac_stream->codecpar->sample_rate = sample_rate;
  aac_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
  aac_stream->codecpar->codec_id = AV_CODEC_ID_AAC;
  aac_stream->codecpar->extradata = reinterpret_cast<uint8_t*>(av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
  aac_stream->codecpar->extradata_size = extradata_size;
  //LOG(VERBOSE) << "extradata size:" << extradata_size;
  aac_stream->codecpar->frame_size = 1024;
  aac_stream->time_base = timebase;
  memcpy(aac_stream->codecpar->extradata, extradata, extradata_size);
  return aac_stream;
}

sp<AVStream> NewAVStream() {
  AVStream *st = reinterpret_cast<AVStream*>(av_malloc(sizeof(AVStream)));
  st->codecpar = avcodec_parameters_alloc();
  CHECK(st != nullptr);
  sp<AVStream> sp_s(st, [](AVStream *stream) {
    avcodec_parameters_free(&stream->codecpar);
    av_freep(&stream);
  });
  return sp_s;
}


sp<AVStream> AddAVStream(const sp<AVFormatContext> &sp_fmt) {
  AVStream *s = avformat_new_stream(sp_fmt.get(), nullptr);
  sp<AVStream> sp_s(s, [](AVStream *stream) {
  });
  return sp_s;
}

#if (__APPLE__)
sp<AVFrame> CopyCVPixelBufferToFrame(CVPixelBufferRef pixbuf) {
  sp<AVFrame> out_frame = NewAVFrame();
  OSType pixel_format = CVPixelBufferGetPixelFormatType(pixbuf);
  CVReturn err;
  uint8_t *data[4] = {0};
  int linesize[4] = {0};

  switch (pixel_format) {
    case kCVPixelFormatType_420YpCbCr8Planar: out_frame->format = AV_PIX_FMT_YUV420P; break;
    case kCVPixelFormatType_422YpCbCr8:       out_frame->format = AV_PIX_FMT_UYVY422; break;
    case kCVPixelFormatType_32BGRA:           out_frame->format = AV_PIX_FMT_BGRA; break;
#ifdef kCFCoreFoundationVersionNumber10_7
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: out_frame->format = AV_PIX_FMT_NV12; break;
#endif
    default:
      LOG(ERROR) << "Unsupported pixel format:" << pixel_format;
      return nullptr;
  }

  out_frame->width  = static_cast<int>(CVPixelBufferGetWidth(pixbuf));
  out_frame->height = static_cast<int>(CVPixelBufferGetHeight(pixbuf));
  auto ret = av_frame_get_buffer(out_frame.get(), 32);
  if (ret < 0)
    return nullptr;

  err = CVPixelBufferLockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly);
  if (err != kCVReturnSuccess) {
    LOG(ERROR) << "Error locking the pixel buffer.";
    return nullptr;
  }

  if (CVPixelBufferIsPlanar(pixbuf)) {
    auto planes = CVPixelBufferGetPlaneCount(pixbuf);
    for (size_t i = 0; i < planes; i++) {
      data[i]     = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixbuf, i);
      linesize[i] = static_cast<int>(CVPixelBufferGetBytesPerRowOfPlane(pixbuf, i));
    }
  } else {
    data[0] = (uint8_t *)CVPixelBufferGetBaseAddress(pixbuf);
    linesize[0] = static_cast<int>(CVPixelBufferGetBytesPerRow(pixbuf));
  }

  av_image_copy(out_frame->data, out_frame->linesize,
                (const uint8_t **)data, linesize, (AVPixelFormat)out_frame->format,
                out_frame->width, out_frame->height);

  CVPixelBufferUnlockBaseAddress(pixbuf, kCVPixelBufferLock_ReadOnly);
  if (ret < 0)
    return nullptr;
  return out_frame;
}

static void videotoolbox_buffer_release(void *opaque, uint8_t *data) {
  CVPixelBufferRef cv_buffer = reinterpret_cast<CVPixelBufferRef>(data);
  CVPixelBufferRelease(cv_buffer);
//  LOG(VERBOSE) << "fuck ......................!!!! release .";
}

sp<AVFrame> RefCVPixelBufferToFrame(CVPixelBufferRef pixel) {
  sp<AVFrame> out_frame = NewAVFrame();
  out_frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
  out_frame->width = static_cast<int>(CVPixelBufferGetWidth(pixel));
  out_frame->height = static_cast<int>(CVPixelBufferGetHeight(pixel));
  out_frame->buf[0] = av_buffer_create((uint8_t*)pixel,
                                       sizeof(pixel),
                                       videotoolbox_buffer_release,
                                       nullptr,
                                       AV_BUFFER_FLAG_READONLY);
  if (!out_frame->buf[0]) {
    return nullptr;
  }
  out_frame->data[3] = (uint8_t*)pixel;
  return out_frame;
}

#endif

sp<AVFormatContext> NewOutputAVFormatContext(const std::string &format_name, const std::string &filename) {
  AVFormatContext *fmt_ctx = nullptr;
  auto ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, format_name.c_str(), filename.c_str());
  CHECK(ret >= 0);
  sp<AVFormatContext> sp_fmt_ctx(fmt_ctx, [](AVFormatContext *s) {
    avformat_free_context(s);
  });
  return sp_fmt_ctx;
}

std::string FFmpegErrorToString(int code) {
  char str[AV_ERROR_MAX_STRING_SIZE] = {0};
  return std::string(av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, code));
}

sp<AVCodecContext> NewAVCodecContext(AVCodec *codec) {
  CHECK(codec != nullptr);
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  CHECK(ctx != nullptr) << "alloc context3 retrun nullptr";
  sp<AVCodecContext> sp_ctx(ctx, [](AVCodecContext *c){
    avcodec_close(c);
    avcodec_free_context(&c);
  });
  return sp_ctx;
}

sp<AVCodecContext> NewAVDecodeContextFromID(AVCodecID decoder_id) {
  auto codec = avcodec_find_decoder(decoder_id);
  if (codec == nullptr) {
    LOG(WARNING) << "unable to find decoder ID:" << decoder_id;
    return nullptr;
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  sp<AVCodecContext> sp_ctx(ctx, [](AVCodecContext *c){
    avcodec_close(c);
    avcodec_free_context(&c);
  });
  return sp_ctx;
}

sp<AVCodecContext> NewAVDecodeContextFromName(const std::string &name) {
  auto codec = avcodec_find_decoder_by_name(name.c_str());
  if (codec == nullptr) {
    LOG(WARNING) << "unable to find decoder:" << name;
    return nullptr;
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  sp<AVCodecContext> sp_ctx(ctx, [](AVCodecContext *c){
    avcodec_close(c);
    avcodec_free_context(&c);
  });
  return sp_ctx;
}

sp<AVCodecContext> NewAVEncodeContextFromID(AVCodecID id) {
  auto codec = avcodec_find_encoder(id);
  if (codec == nullptr) {
    LOG(WARNING) << "unable to find encoder, ID:" << id;
    return nullptr;
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  sp<AVCodecContext> sp_ctx(ctx, [](AVCodecContext *c){
    avcodec_close(c);
    avcodec_free_context(&c);
  });
  return sp_ctx;
}

sp<AVCodecContext> NewAVHwaccelEncodeContextFromID(AVCodecID id) {
#if _WIN32
  if (id == AV_CODEC_ID_H264) return NewAVEncodeContextFromName("h264_nvenc");
#elif __APPLE__
  if (id == AV_CODEC_ID_H264) return NewAVEncodeContextFromName("h264_videotoolbox");
#endif // _WIN32
  return nullptr;
}

sp<AVCodecContext> NewAVHwaccelDecodeContextFromID(AVCodecID id) {
#if (__APPLE__)
  if (id == AV_CODEC_ID_AAC) {
    return NewAVDecodeContextFromName("aac_at");
  }
  //atempt use videotoolbox, this must be after avcodec_parameters_to_context method.
  if (id == AV_CODEC_ID_H264) {
    auto ctx = NewAVDecodeContextFromID(AV_CODEC_ID_H264);
    if (ctx) {
      ctx->pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
      ctx->get_format = UseAVPixlFmtVideoToolbox;
    }
    return ctx;
  }
#elif (_WIN32)
  if (id == AV_CODEC_ID_H264) {
    return NewAVDecodeContextFromName("h264_cuvid");
  }
#endif
  return nullptr;
}

sp<AVCodecContext> NewAVEncodeContextFromName(const std::string &name) {
  auto codec = avcodec_find_encoder_by_name(name.c_str());
  if (codec == nullptr) {
    LOG(WARNING) << "unable to find encoder name:" << name;
    return nullptr;
  }
  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  sp<AVCodecContext> sp_ctx(ctx, [](AVCodecContext *c){
    avcodec_close(c);
    avcodec_free_context(&c);
  });
  return sp_ctx;
}

}


