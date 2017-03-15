//
// Created by jerett on 16/8/29.
//

#ifndef INSPLAYER_MEDIA_SRC_H
#define INSPLAYER_MEDIA_SRC_H

#include <functional>
#include "filter/media_filter.h"
#include <av_toolbox/sp.h>

namespace ins {

class MediaSrc: public std::enable_shared_from_this<MediaSrc> {
public:
  enum MediaSrcState {
    MediaSourceEnd = 0,
    MediaSourceError,
  };
  using STATE_CALLBACK = std::function<void(MediaSrcState state)>;

  virtual ~MediaSrc() = default;

  template <typename FilterType>
  FilterType& set_video_filter(const FilterType &filter) noexcept {
    video_filter_ = filter;
    return const_cast<FilterType&>(filter);
  }

  template <typename FilterType>
  FilterType& set_audio_filter(const FilterType &filter) noexcept {
    audio_filter_ = filter;
    return const_cast<FilterType&>(filter);
  }
  
  /// register state callback
  /// \param state_callback
  void RegisterCallback(const STATE_CALLBACK &state_callback) noexcept {
    state_callback_ = state_callback;
  }
  
  virtual double progress() const = 0;
  virtual bool eof() const = 0;
  virtual bool Prepare() = 0;
  virtual void Start() = 0;
  virtual void Cancel() = 0;
  virtual void Close() = 0;

protected:
  STATE_CALLBACK state_callback_;
  sp<PacketSource> video_filter_ = nullptr;
  sp<PacketSource> audio_filter_ = nullptr;
};

}

#endif //INSPLAYER_MEDIA_SRC_H
