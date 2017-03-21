//
// Created by jerett on 16/12/19.
//

#ifndef INSPLAYER_LOGFILTER_H
#define INSPLAYER_LOGFILTER_H

#include "media_filter.h"
#include <llog/llog.h>

namespace ins {

template <typename T>
class LogFilter : public MediaSink<T> {
public:
  LogFilter(const std::string &prefix) noexcept : prefix_(prefix) {}
  ~LogFilter() = default;

  bool Prepare() override {
    return true;
  }

  bool Init(MediaBus &bus) override {
    return true;
  }

  bool Filter(const T &data) override {
//    LOG(INFO) << prefix_ << data->pts;
    return true;
  }


  void Close() override {
    ;
  }

private:
  std::string prefix_;

};


}

#endif //INSPLAYER_LOGFILTER_H
