//
// Created by jerett on 16/12/7.
//

#ifndef INSMEDIAUTIL_DEMUXER_H
#define INSMEDIAUTIL_DEMUXER_H

#include <string>
#include <map>
#include "sp.h"

struct AVStream;
struct AVFormatContext;

namespace ins {

struct ARVPacket;

class Demuxer {
public:
  explicit Demuxer(const std::string &file) noexcept;
  ~Demuxer();

  int Open(std::map<std::string, std::string> *options = nullptr);

  const AVFormatContext *fmt_ctx() const;
  const AVStream* video_stream() const;
  const AVStream* audio_stream() const;

  const AVStream* IndexOfStream(unsigned int index) const;
  int64_t video_duration() const;
  int64_t audio_duration() const;
  int64_t duration() const;

  int video_stream_index() const {
    return video_stream_index_;
  }

  int audio_stream_index() const {
    return audio_stream_index_;
  }

  int subtitle_stream_index() const {
    return subtitle_stream_index_;
  }

  int NextPacket(sp<ARVPacket> &pkt);

  int Seek(int stream_index, int64_t timestamp, int flags);

  void interrupt() {
    io_interrupt_ = true;
  }

  void Close();

private:
  static int InterruptCallback(void*);

private:
  bool closed_ = false;
  bool io_interrupt_ = false;
  int video_stream_index_;
  int audio_stream_index_;
  int subtitle_stream_index_;
  std::string file_;
  AVFormatContext *fmt_ctx_ = nullptr;
};

}


#endif //INSMEDIAUTIL_DEMUXER_H
