//
// Created by jerett on 16/12/23.
//

#include "scaler.h"
extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
#include <llog/llog.h>
#include "ffmpeg_util.h"
#include "platform.h"

namespace ins {

Scaler::Scaler(int out_width, int out_height, AVPixelFormat out_fmt, int algorithm) noexcept
  : out_width_(out_width), out_height_(out_height), algorithm_(algorithm), out_fmt_(out_fmt) {
}

Scaler::~Scaler() {
  if (sws_ctx_ != nullptr) {
    sws_freeContext(sws_ctx_);
  }
}

bool Scaler::Init(const sp <AVFrame> &src_frame) {
  if (sws_ctx_ == nullptr) {
    sp<AVFrame> param_frame = src_frame;
    #if (__APPLE__)
    if (src_frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
      CVPixelBufferRef pixel_buffer = (CVPixelBufferRef)src_frame->data[3];
      param_frame = CopyCVPixelBufferToFrame(pixel_buffer);
      av_frame_copy_props(param_frame.get(), src_frame.get());
    }
    sws_ctx_ = sws_getContext(param_frame->width, param_frame->height, static_cast<AVPixelFormat>(param_frame->format),
                              out_width_, out_height_, out_fmt_,
                              algorithm_, nullptr, nullptr, nullptr);
    #else
    sws_ctx_ = sws_getContext(param_frame->width, param_frame->height, static_cast<AVPixelFormat>(param_frame->format),
                              out_width_, out_height_, out_fmt_,
                              algorithm_, nullptr, nullptr, nullptr);
    #endif
  }
  return sws_ctx_ != nullptr;
}

int Scaler::ScaleFrame(const sp<AVFrame> &src_frame, sp<AVFrame> &out_frame) {
  sp<AVFrame> in_frame = src_frame;
  #if (__APPLE__)
  if (src_frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
    CVPixelBufferRef pixel_buffer = (CVPixelBufferRef)src_frame->data[3];
    in_frame = CopyCVPixelBufferToFrame(pixel_buffer);
    av_frame_copy_props(in_frame.get(), src_frame.get());
  }
  #endif
  auto scale_frame = NewAVFrame(out_width_, out_height_, out_fmt_);
  auto ret = sws_scale(sws_ctx_, in_frame->data, in_frame->linesize, 0, in_frame->height,
                       scale_frame->data, scale_frame->linesize);
  if (ret < 0) {
    LOG(ERROR) << "scale err:" << FFmpegErrorToString(ret);
    out_frame = nullptr;
  } else {
    av_frame_copy_props(scale_frame.get(), in_frame.get());
    out_frame = scale_frame;
  }
  return ret;
}

}
