//
// Created by jerett on 16/12/3.
//

#include "encoder.h"
#include "ffmpeg_util.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <llog/llog.h>

namespace ins {

int Encoder::Open() {
  auto ret = avcodec_open2(enc_ctx_.get(), enc_ctx_->codec, nullptr);
  LOG_IF(ERROR, ret < 0) << "avcodec open encoder:" << avcodec_get_name(enc_ctx_->codec_id)
                         << " failed:" << FFmpegErrorToString(ret);
  return ret;
}

int Encoder::SendFrame(const sp<AVFrame> &in_frame) {
  CHECK(in_frame != nullptr);
  auto dst_frame = CloneAVFrame(in_frame);
  dst_frame->pict_type = AV_PICTURE_TYPE_NONE;
//  LOG(INFO) << "clone frame pts:" << dst_frame->pts << " pic_type:" << dst_frame->pict_type ;
  auto ret = avcodec_send_frame(enc_ctx_.get(), dst_frame.get());
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN)) {
      LOG(WARNING) << "send frame err EAGAIN, you should receive pkt first";
    } else {
      LOG(WARNING) << "send frame err:" << FFmpegErrorToString(ret);
    }
  }
  return ret;
}

int Encoder::SendEOF() {
  return avcodec_send_frame(enc_ctx_.get(), nullptr);
}

int Encoder::ReceivePacket(sp<ARVPacket> &out_pkt) {
  auto pkt = NewPacket();
  auto ret = avcodec_receive_packet(enc_ctx_.get(), pkt.get());
  if (ret < 0) {
    pkt = nullptr;
    if (ret == AVERROR(EAGAIN)) {
      LOG(WARNING) << "receive pkt err EAGAIN, output is not availble now";
    } else {
      LOG(WARNING) << "receive pkt warning:" << FFmpegErrorToString(ret);
    }
  }
  out_pkt = pkt;
  if (out_pkt) {
    out_pkt->media_type = enc_ctx_->codec_type;
  }
  return ret;
}

}
