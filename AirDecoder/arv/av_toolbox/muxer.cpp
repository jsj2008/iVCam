//
// Created by jerett on 16/12/6.
//

#include "muxer.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}
#include "ffmpeg_util.h"
#include <llog/llog.h>

namespace ins {

Muxer::Muxer(const sp<AVFormatContext> &ctx) noexcept : mux_ctx_(ctx) {
  interrupt_cb_ = std::make_shared<AVIOInterruptCB>();
  CHECK(interrupt_cb_ != nullptr);
  interrupt_cb_->callback = InterruptCallback;
  interrupt_cb_->opaque = this;
}

Muxer::~Muxer() {
  Close();
}

sp<AVStream>& Muxer::GetVideoStream() {
  if (video_stream_ == nullptr) {
    video_stream_ = AddAVStream(mux_ctx_);
  }
  return video_stream_;
}

sp<AVStream>& Muxer::GetAudioStream() {
  if (audio_stream_ == nullptr) {
    audio_stream_ = AddAVStream(mux_ctx_);
  }
  return audio_stream_;
}

int Muxer::InterruptCallback(void* opaque) {
  ins::Muxer *muxer = reinterpret_cast<ins::Muxer*>(opaque);
  return muxer->io_interrupt_;
}

int Muxer::Open() {
  std::lock_guard<std::mutex> lck(mtx_);
  if (!(mux_ctx_->oformat->flags & AVFMT_NOFILE)) {
    auto ret = avio_open2(&mux_ctx_->pb, mux_ctx_->filename, AVIO_FLAG_WRITE, interrupt_cb_.get(), nullptr);
    if (ret < 0) {
      LOG(ERROR) << "avio open error:" << FFmpegErrorToString(ret);
      if (!(mux_ctx_->oformat->flags & AVFMT_NOFILE)) {
        avio_close(mux_ctx_->pb);
      }
      return ret;
    }
  }

  auto ret = avformat_write_header(mux_ctx_.get(), nullptr);
  if (ret < 0) {
    LOG(ERROR) << "writer header error:" << FFmpegErrorToString(ret);
    return ret;
  }
  av_dump_format(mux_ctx_.get(), 0, mux_ctx_->filename, 1);
  open_ = true;
  return 0;
}

int Muxer::WriteVideoPacket(const sp<AVPacket> &pkt, const AVRational &src_timebase) {
  av_packet_rescale_ts(pkt.get(), src_timebase, video_stream_->time_base);
  pkt->stream_index = video_stream_->index;
  return WritePacket(pkt);
}

int Muxer::WriteAudioPacket(const sp<AVPacket> &pkt, const AVRational &src_timebase) {
  av_packet_rescale_ts(pkt.get(), src_timebase, audio_stream_->time_base);
  pkt->stream_index = audio_stream_->index;
  return WritePacket(pkt);
}

void Muxer::Interrupt() {
  io_interrupt_ = true;
}

int Muxer::WritePacket(const sp<AVPacket> &pkt) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto ret = av_interleaved_write_frame(mux_ctx_.get(), pkt.get());
  if (ret != 0) {
    LOG(ERROR) << "write frame err:" << FFmpegErrorToString(ret);
  }
  return ret;
}

int Muxer::Close() {
  std::lock_guard<std::mutex> lck(mtx_);
  if (open_) {
    open_ = false;
    auto ret = av_write_trailer(mux_ctx_.get());
    if (ret != 0) {
      LOG(ERROR) << "write trailer error:" << FFmpegErrorToString(ret);
      return ret;
    }
    if (!(mux_ctx_->oformat->flags & AVFMT_NOFILE)) {
      avio_close(mux_ctx_->pb);
    }
    mux_ctx_.reset();
    LOG(VERBOSE) << "muxer close.";
  }
  return true;
}

}
