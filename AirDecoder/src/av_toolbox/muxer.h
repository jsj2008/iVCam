//
// Created by jerett on 16/12/6.
//

#ifndef INSMEDIAUTIL_MUXER_H
#define INSMEDIAUTIL_MUXER_H

#include "sp.h"

#include <mutex>

struct AVStream;
struct AVPacket;
struct AVFormatContext;
struct AVIOInterruptCB;
struct AVRational;

namespace ins {
class Muxer {
public:
  explicit Muxer(const sp<AVFormatContext> &ctx) noexcept;
  ~Muxer();

  /// get video stream, if none, then create one
  /// \return
  sp<AVStream>& GetVideoStream();

  /// get audio stream, if none, then create one
  /// \return
  sp<AVStream>& GetAudioStream();

  int Open();
  int WriteVideoPacket(const sp<AVPacket> &pkt, const AVRational &src_timebase);
  int WriteAudioPacket(const sp<AVPacket> &pkt, const AVRational &src_timebase);
  void Interrupt();
  int Close();

  sp<AVFormatContext>& mux_ctx() {
    return mux_ctx_;
  }

private:
  static int InterruptCallback(void *p);
  int WritePacket(const sp<AVPacket> &pkt);

private:
  sp<AVStream> video_stream_;
  sp<AVStream> audio_stream_;
  std::mutex mtx_;

  sp<AVIOInterruptCB> interrupt_cb_;
  bool io_interrupt_ = false;
  sp<AVFormatContext> mux_ctx_;
  bool open_ = false;
};

}


#endif //INSMEDIAUTIL_MUXER_H
