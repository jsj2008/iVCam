//
// Created by jerett on 16/12/23.
//

#ifndef INSPLAYER_SCALE_FILTER_H
#define INSPLAYER_SCALE_FILTER_H

#include "av_toolbox/scaler.h"
#include "media_filter.h" 

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace ins {

class ScaleFilter : public VideoImageFilter<sp<AVFrame>> {
public:
  ///
  /// \param out_width  scale out width
  /// \param out_height scale out height
  /// \param out_fmt  scale pix fmt
  /// \param algorithm scale alogrithm, recommand using SWS_FAST_BILINEAR, 
  /// ref:<libswscale/swscale.h> or http://blog.csdn.net/leixiaohua1020/article/details/12029505
  ScaleFilter(int out_width, int out_height, AVPixelFormat out_fmt, int algorithm) {
    scaler_ = std::make_shared<Scaler>(out_width, out_height, out_fmt, algorithm);
  }
  ~ScaleFilter() = default;

  bool Init(MediaBus &bus) override {
    bus.out_codecpar->width = scaler_->out_width();
    bus.out_codecpar->height = scaler_->out_height();
    bus.out_codecpar->format = scaler_->out_fmt();
    return this->next_filter_->Init(bus);
  }

  bool Filter(const sp<AVFrame> &frame) override {
    if (!scaler_->Init(frame)) return false;
    sp<AVFrame> scale_frame;
    auto ret = scaler_->ScaleFrame(frame, scale_frame);
      
    return ret > 0 && next_filter_->Filter(scale_frame);
  }

  void Close() override {
    return next_filter_->Close();
  }

private:
  sp<Scaler> scaler_;
};

}

#endif //INSPLAYER_SCALE_FILTER_H
