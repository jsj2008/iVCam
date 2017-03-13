//
// Created by jerett on 16/7/14.
//

#ifndef INSPLAYER_MEDIA_FILTER_H
#define INSPLAYER_MEDIA_FILTER_H

#include <string>
#include <memory>
#include <av_toolbox/sp.h>

struct AVStream;
struct AVCodecParameters;
struct AVPacket;
struct AVFrame;

namespace ins {

struct ARVPacket;
class any;

struct MediaBus {
  //read only, pointer will invalid when media src destruct
  const AVStream *in_stream = nullptr;
  sp<AVCodecParameters> out_codecpar;
};

template <typename InDataType>
class MediaSource {
public:
  using in_type = InDataType;
  MediaSource() = default;
  virtual ~MediaSource() = default;

  virtual bool Filter(const InDataType&) = 0;
  virtual void Close() = 0;
  virtual bool Init(MediaBus &bus) = 0;
  virtual void SetOpt(const std::string &key, const any &value) {}
};

template <typename InDataType, typename OutDataType>
class MediaFilter: public MediaSource<InDataType> {
  template <typename DataType>
  using SpDataFilter = sp<MediaSource<DataType>>;

public:
  using out_type = OutDataType;

  template <typename NextFilterType>
  NextFilterType& set_next_filter(NextFilterType &next_filter) noexcept {
    next_filter_ = next_filter;
    return next_filter;
  }

protected:
  SpDataFilter<OutDataType> next_filter_;
};

template <typename InDataType>
class MediaSink: public MediaSource<InDataType> {
public:
  /// Need prepare first. for the reason Init function may be called many times
  /// \return
  virtual bool Prepare() = 0;
};

using PacketSource = MediaSource<sp<ARVPacket>>;
using FrameSource = MediaSource<sp<AVFrame>>;
using PacketSink = MediaSink<sp<ARVPacket>>;

template <typename OutDataType>
using StreamFilter = MediaFilter<sp<ARVPacket>, OutDataType>;

template <typename OutDataType>
using VideoImageFilter = MediaFilter<sp<AVFrame>, OutDataType>;

}


#endif //INSPLAYER_MEDIA_FILTER_H
