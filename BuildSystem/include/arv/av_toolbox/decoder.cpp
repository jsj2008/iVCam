//
// Created by jerett on 16/12/2.
//

#include "decoder.h"
extern "C" {
#if (__APPLE__)
#include <libavcodec/videotoolbox.h>
#endif
#include <libavcodec/avcodec.h> 
}
#include <llog/llog.h>
#include "ffmpeg_util.h"

namespace ins {

Decoder::Decoder(const sp<AVCodecContext> &dec_ctx) noexcept : dec_ctx_(dec_ctx) {
  CHECK(dec_ctx_ != nullptr);
}

int Decoder::Open() {
  auto ret = avcodec_open2(dec_ctx_.get(), dec_ctx_->codec, nullptr);
  LOG_IF(ERROR, ret < 0) << "avcodec open failed: " << FFmpegErrorToString(ret);
  return ret;
}

int Decoder::SendPacket(const sp<AVPacket> &in_pkt) {
  CHECK(in_pkt != nullptr);
  auto ret = avcodec_send_packet(dec_ctx_.get(), in_pkt.get());
  if (ret != 0) {
    LOG(WARNING) << "send_packet:" << FFmpegErrorToString(ret);
  }
  return ret;
}

int Decoder::SendEOF() {
  LOG(VERBOSE) << "Send EOF packet";
  return avcodec_send_packet(dec_ctx_.get(), nullptr);
}

int Decoder::ReceiveFrame(sp<AVFrame> &out_frame) {
  auto frame = NewAVFrame();
  auto ret = avcodec_receive_frame(dec_ctx_.get(), frame.get());
  if (ret == 0) {
    //if (frame && filter_) {
    //  out_frame = filter_(frame);
    //} else {
    //  out_frame = frame;
    //}
    out_frame = frame;

  } else if (ret == AVERROR(EAGAIN)) {
    LOG(VERBOSE) << "receive frame err egain, need to send new input";
  } else if (ret == AVERROR_EOF) {
    LOG(VERBOSE) << "receive ReceiveFrameframe err eof";
  } else {
    LOG(VERBOSE) << "receive frame err:" << FFmpegErrorToString(ret);
  }
  return ret;
}

void Decoder::FlushBuffer() {
#if (TARGET_OS_IPHONE | TARGET_OS_MAC)
  if (dec_ctx_->pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX) {
    av_videotoolbox_default_free(dec_ctx_.get());
  }
#endif
  avcodec_flush_buffers(dec_ctx_.get());
}

Decoder::~Decoder() {
#if (TARGET_OS_IPHONE | TARGET_OS_MAC)
  if (dec_ctx_->pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX) {
    av_videotoolbox_default_free(dec_ctx_.get());
  }
#endif
  dec_ctx_ = nullptr;
}

}
