//
// Created by jerett on 16/12/3.
//

#ifndef INSMEDIAUTIL_VIDEO_ENCODER_H
#define INSMEDIAUTIL_VIDEO_ENCODER_H

#include "sp.h"

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace ins {
struct ARVPacket;

class Encoder {
public:
  explicit Encoder(const sp<AVCodecContext> &enc_ctx) noexcept : enc_ctx_(enc_ctx) {}
  int Open();
  int SendFrame(const sp<AVFrame> &in_frame);
  int SendEOF();
  int ReceivePacket(sp<ARVPacket> &out_pkt);

  sp<AVCodecContext> enc_ctx() const {
    return enc_ctx_;
  }

private:
  sp<AVCodecContext> enc_ctx_;
};

}
#endif //INSMEDIAUTIL_VIDEO_ENCODER_H
