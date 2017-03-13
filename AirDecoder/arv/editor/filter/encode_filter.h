//
// Created by jerett on 16/8/11.
//

#ifndef INSPLAYER_ENCODE_FILTER_H
#define INSPLAYER_ENCODE_FILTER_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <llog/llog.h>
#include <memory>
#include <string>
#include <av_toolbox/sp.h>
#include <av_toolbox/encoder.h>
#include "media_filter.h"

struct AVFrame;
struct AVPacket;

///test video encode only now.
namespace ins {
//set encode bitrate. value type: int
const static std::string kEncodeBitrate("bitrate");
//set  bframes number. value type: int
const static std::string kEncodeBFrames("bframes");
//set encode gop size. value type:int
const static std::string kEncodeGopSize("gop_size");
//set encode threads. value type: int
const static std::string kEncodeThreads("threads");
//set encode preset. value type: string
const static std::string kX264EncodePreset("preset");
//set encode tune. value type: string
const static std::string kX264EncodeTune("tune");
//indicate whether use haradware encode.
//value type: bool, default true
const static std::string kUseHwaccelEncode("use_hwaccel_encode");


class EncodeFilter : public VideoImageFilter<sp<ARVPacket>> {
public:
  EncodeFilter(AVCodecID encodec_id) noexcept : encodec_id_(encodec_id) {}
  ~EncodeFilter() = default;

  void SetOpt(const std::string &key, const any &value) override;
  bool Init(MediaBus &bus) override;
  bool Filter(const sp<AVFrame> &frame) override;
  void Close() override;

private:
  bool CreateHwaccelEncoder(const MediaBus &bus);
  bool CreateSoftwareEncoder(const MediaBus &bus);
  bool ConfigureEncContext(const MediaBus &bus);

private:
  int stream_index_ = -1;
  int bitrate_ = -1;
  int bframes_ = -1;
  int gop_size_ = -1;
  int threads_ = -1;
  bool enable_hwaccel_ = true;
  std::string preset_;
  std::string tune_;
  AVCodecID encodec_id_;
  sp<AVCodecContext> enc_ctx_;
  up<Encoder> encoder_;
};

}


#endif //INSPLAYER_X264_ENCODE_FILTER_H
