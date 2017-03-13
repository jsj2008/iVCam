//
// Created by jerett on 16/7/18.
//

#ifndef INSPLAYER_DECODE_FILTER_H
#define INSPLAYER_DECODE_FILTER_H

#include <memory>
extern "C" {
#include <libavutil/rational.h>
}
#include "media_filter.h"
#include <av_toolbox/decoder.h>

struct AVCodecContext;

namespace ins {

//indicate whether use haradware decode.
//value type: bool, default true
const static std::string kUseHwaccelDecode("use_hwaccel_decode"); 

class DecodeFilter : public StreamFilter<sp<AVFrame>> {
public:
  DecodeFilter() noexcept = default;
  ~DecodeFilter() = default;

  void SetOpt(const std::string &key, const any& val) override;
  bool Init(MediaBus &bus) override;
  void Close() override;
  bool Filter(const sp<ARVPacket> &pkt) override;
  bool enable_hwaccel() const {
    return enable_hwaccel_;
  }
 
private:
  bool CreateHardwareDecoder(const AVStream *stream);
  bool CreateSoftwareDecoder(const AVStream *stream);

private:
  bool enable_hwaccel_ = true;
  AVRational time_base_;
//  AVStream *stream_ = nullptr;
  sp<AVCodecContext> dec_ctx_;
  up<Decoder> decoder_ = nullptr;
};

}


#endif //INSPLAYER_DECODE_FILTER_H
