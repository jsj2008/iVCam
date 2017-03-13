//
// Created by jerett on 16/12/23.
//

#ifndef INSPLAYER_SWSCALER_H
#define INSPLAYER_SWSCALER_H

extern "C" {
#include <libavutil/pixfmt.h>
}
#include "sp.h"

struct AVFrame;
struct SwsContext;

namespace ins {

class Scaler {
public:
  Scaler(int out_width, int out_height, AVPixelFormat out_fmt, int algorithm) noexcept;
  ~Scaler();

  int out_width() const {
    return out_width_;
  }

  int out_height() const {
    return out_height_;
  }

  AVPixelFormat out_fmt() const {
    return out_fmt_;
  }

  bool Init(const sp<AVFrame> &src_frame);
  int ScaleFrame(const sp<AVFrame> &src_frame, sp<AVFrame> &out_frame);

private:
  int out_width_;
  int out_height_;
  int algorithm_;
  AVPixelFormat out_fmt_;
  SwsContext *sws_ctx_ = nullptr;
};

}



#endif //INSPLAYER_SWSCALER_H
