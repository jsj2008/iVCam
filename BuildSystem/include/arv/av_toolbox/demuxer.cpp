//
// Created by jerett on 16/12/7.
//

#include "demuxer.h"
#include "ffmpeg_util.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#include <llog/llog.h>

namespace ins {

Demuxer::Demuxer(const std::string &file) noexcept : file_(file) {
  ;
}

ins::Demuxer::~Demuxer() {
  Close();
}

const AVFormatContext* Demuxer::fmt_ctx() const {
  return fmt_ctx_;
}

const AVStream* Demuxer::video_stream() const {
  if (fmt_ctx_ && video_stream_index_ >= 0) return fmt_ctx_->streams[video_stream_index_];
  return nullptr;
}

const AVStream* Demuxer::audio_stream() const {
  if (fmt_ctx_ && audio_stream_index_ >= 0) return fmt_ctx_->streams[audio_stream_index_];
  return nullptr;
}

const AVStream* Demuxer::IndexOfStream(unsigned int index) const {
  if (fmt_ctx_ && index < fmt_ctx_->nb_streams) return fmt_ctx_->streams[index];
  return nullptr;
}

int64_t Demuxer::video_duration() const {
  auto video_st = video_stream();
  if (fmt_ctx_ && video_st && video_st->duration >= 0) {
    return TimestampToMs(video_st->duration, video_st->time_base);
  } else {
    return -1;
  }
}

int64_t Demuxer::audio_duration() const {
  auto audio_st = audio_stream();
  if (fmt_ctx_ && audio_st && audio_st->duration >= 0) {
    return TimestampToMs(audio_st->duration, audio_st->time_base);
  } else {
    return -1;
  }
}

int64_t Demuxer::duration() const {
  if (fmt_ctx_) {
    return TimestampToMs(fmt_ctx_->duration, {1, AV_TIME_BASE});
  } else {
    return -1;
  }
}

int Demuxer::InterruptCallback(void* p) {
  auto self = reinterpret_cast<Demuxer*>(p);
  return self->io_interrupt_;
}

int Demuxer::Open(std::map<std::string, std::string> *options) {
  AVDictionary *opt_dic = nullptr;
  if (options) {
    for (auto &opt : *options) {
      av_dict_set(&opt_dic, opt.first.c_str(), opt.second.c_str(), 0);
    }
  }

  fmt_ctx_ = avformat_alloc_context();
  fmt_ctx_->interrupt_callback.callback = InterruptCallback;
  fmt_ctx_->interrupt_callback.opaque = this;
  auto ret = avformat_open_input(&fmt_ctx_, file_.c_str(), nullptr, &opt_dic);
  if (ret == 0) {
    avformat_find_stream_info(fmt_ctx_, nullptr);
    video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    subtitle_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);
    av_dump_format(fmt_ctx_, 0, fmt_ctx_->filename, 0);
    LOG(VERBOSE) << "is seekable:" << fmt_ctx_->pb->seekable;
  } else {
//    LOG(ERROR) << "avformat open input error:" << FFmpegErrorToString(ret);
    LOG(ERROR) << "avformat open input error:" << ins::FFmpegErrorToString(ret);
  }
//  LOG(VERBOSE) << "avformat duration:" << fmt_ctx_->duration;
//  LOG(VERBOSE) << "video duration:"
//    << av_rescale_q(video_stream()->duration, video_stream()->time_base, AV_TIME_BASE_Q);
//  if (audio_stream()) {
//    LOG(VERBOSE) << "audio duration:"
//      << av_rescale_q(audio_stream()->duration, audio_stream()->time_base, AV_TIME_BASE_Q);
//  }
  return ret;
}

void Demuxer::Close() {
  if (!closed_) {
    if (fmt_ctx_ != nullptr) {
      avformat_close_input(&fmt_ctx_);
    }
  }
}

int Demuxer::Seek(int stream_index, int64_t timestamp, int flags) {
  return av_seek_frame(fmt_ctx_, stream_index, timestamp, flags);
}

int Demuxer::NextPacket(sp<ARVPacket> &pkt) {
  AVPacket tmp_pkt;
  av_init_packet(&tmp_pkt);
  auto ret = av_read_frame(fmt_ctx_, &tmp_pkt);
  if (ret == 0) {
    pkt = NewPacket();
    av_packet_ref(pkt.get(), &tmp_pkt);
    av_packet_unref(&tmp_pkt);
    if (pkt->stream_index == video_stream_index_) {
      pkt->media_type = AVMEDIA_TYPE_VIDEO;
    } else if (pkt->stream_index == audio_stream_index_) {
      pkt->media_type = AVMEDIA_TYPE_AUDIO;
    } else if (pkt->stream_index == subtitle_stream_index_) {
      pkt->media_type = AVMEDIA_TYPE_SUBTITLE;
    }
  } else {
    pkt = nullptr;
  }
  return ret;
}

}
