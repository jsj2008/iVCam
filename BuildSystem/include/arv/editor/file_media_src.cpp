//
//  file_media_source.cpp
//  INSMediaApp
//
//  Created by jerett on 16/7/13.
//  Copyright © 2016 Insta360. All rights reserved.
//

#include "file_media_src.h"
extern "C" {
#include <libavformat/avformat.h>
}
#include <llog/llog.h>
#include <chrono>
#include <algorithm>
#include <av_toolbox/demuxer.h>
#include <av_toolbox/ffmpeg_util.h>
#include <av_toolbox/any.h>
#include "filter/media_filter.h"

namespace ins {

FileMediaSrc::FileMediaSrc(const std::string &url) : file_url_(url) {
  ;
}

FileMediaSrc::~FileMediaSrc() {
  LOG(VERBOSE) << "~FileMediaSrc:" << file_url_;
}

void FileMediaSrc::SetOpt(const std::string &key, const any &val) {
  if (key == kSrcMode) {
    auto mode = any_cast<std::string>(val);
    if (mode == "re") {
      realtime_ = true;
      LOG(VERBOSE) << "go into realtime mode.";
    }
 }
}

void FileMediaSrc::set_trim_range(int64_t start_ms, int64_t end_ms) noexcept {
  std::get<0>(range_) = true;
  std::get<1>(range_) = start_ms;
  std::get<2>(range_) = end_ms;
}

const AVStream* FileMediaSrc::video_stream() const {
  return demuxer_->video_stream();
}

const AVStream* FileMediaSrc::audio_stream() const {
  return demuxer_->audio_stream();
}

int64_t FileMediaSrc::video_duration() const {
  return demuxer_->video_duration();
}

int64_t FileMediaSrc::audio_duration() const {
  return demuxer_->audio_duration();
}

void FileMediaSrc::Loop() {
  using namespace std::chrono;
  auto need_range = std::get<0>(range_);
  auto left_pos_ms = std::get<1>(range_);
  auto right_pos_ms = std::get<2>(range_);

  auto duration_ms = std::max(demuxer_->video_duration(), demuxer_->audio_duration());
  if (need_range) {
    demuxer_->Seek(-1, MsToTimestamp(left_pos_ms, { 1, AV_TIME_BASE }), AVSEEK_FLAG_BACKWARD);
    duration_ms = std::min(right_pos_ms, duration_ms) - left_pos_ms;
  }

  int64_t video_start_dts_ = INT_MIN;
  int64_t audio_start_dts_ = INT_MIN;

  auto start_clock = steady_clock::now();
  while (!stop_) {
    sp<ARVPacket> pkt;
    auto ret = demuxer_->NextPacket(pkt);

    switch(ret) {
      case 0:
      {
        if (pkt->stream_index == demuxer_->video_stream_index()) {
          if (video_start_dts_ == INT_MIN && audio_start_dts_ == INT_MIN) {
            video_start_dts_ = pkt->dts;
            if (demuxer_->audio_stream_index() >= 0) {
              audio_start_dts_ = av_rescale_q(video_start_dts_,
                                              demuxer_->video_stream()->time_base,
                                              demuxer_->audio_stream()->time_base);
            }
          }
          pkt->pts -= video_start_dts_;
          pkt->dts -= video_start_dts_;
        } else if (pkt->stream_index == demuxer_->audio_stream_index()) {
          if (video_start_dts_ == INT_MIN && audio_start_dts_ == INT_MIN) {
            audio_start_dts_ = pkt->dts;
            if (demuxer_->video_stream_index() >= 0) {
              video_start_dts_ = av_rescale_q(audio_start_dts_,
                                              demuxer_->audio_stream()->time_base,
                                              demuxer_->video_stream()->time_base);
            }
          }
          pkt->pts -= audio_start_dts_;
          pkt->dts -= audio_start_dts_;
        } else {
          continue;
        }

        //pts may be changed by filter, so caculate progress first
        auto cur_pkt_dts_ms = TimestampToMs(pkt->dts, demuxer_->IndexOfStream(pkt->stream_index)->time_base);
        if (realtime_) {
          auto pass_ms = duration_cast<milliseconds>(steady_clock::now() - start_clock);
          if (cur_pkt_dts_ms > pass_ms.count()) {
            auto diff = milliseconds(cur_pkt_dts_ms) - pass_ms;
            std::this_thread::sleep_for(diff);
          }
        }
        if (pkt->stream_index == demuxer_->video_stream_index()) {
//          LOG(VERBOSE) << "cut pkt:" << cur_pkt_dts_ms;
        }

        progress_ = static_cast<double >(cur_pkt_dts_ms) / duration_ms;
//        LOG(VERBOSE) << "progress:" << progress_;
        if (pkt->stream_index == demuxer_->video_stream_index()) {
          if (video_filter_ && !video_filter_->Filter(pkt)) {
            LOG(ERROR) << "video filter err.";
            OnError();
          }
        } else if (pkt->stream_index == demuxer_->audio_stream_index()) {
          if (audio_filter_ && !audio_filter_->Filter(pkt)) {
            LOG(ERROR) << "audio filter err.";
            OnError();
          }
        }
        if (need_range && progress_ >= 1.0) {
          OnEnd();
        }
//        LOG(VERBOSE) << "cur progress:" << progress_;
        break;
      }

      case AVERROR_EOF:
      {
        OnEnd();
        break;
      }

      default:
      {
        LOG(VERBOSE) << "error :" << FFmpegErrorToString(ret);
        OnError();
        break;
      }
    }
  }
  LOG(VERBOSE) << "loop finish .";
}

void FileMediaSrc::OnEnd() {
  LOG(VERBOSE) << "error eof...";
  eof_ = true;
  stop_ = true;
  if (state_callback_) {
    state_callback_(MediaSourceEnd);
  }
}

void FileMediaSrc::OnError() {
  stop_ = true;
  if (state_callback_) {
    state_callback_(MediaSourceError);
  }
}

bool FileMediaSrc::Prepare() {
  demuxer_.reset(new Demuxer(file_url_));
  auto ret = demuxer_->Open(nullptr);
  if (ret != 0) return false;
  stop_ = false;
  eof_ = false;

  //init filter
  if (demuxer_->video_stream_index() >= 0 && video_filter_) {
    MediaBus video_bus;
    auto video_stream = demuxer_->video_stream();
    auto codecpar = NewAVCodecParameters();
    avcodec_parameters_copy(codecpar.get(), video_stream->codecpar);

    video_bus.in_stream = video_stream;
    video_bus.out_codecpar = codecpar;
    if (!video_filter_->Init(video_bus)) return false;
  }

  if (demuxer_->audio_stream_index() >= 0 && audio_filter_) {
    MediaBus audio_bus;
    auto audio_stream = demuxer_->audio_stream();
    auto codecpar = NewAVCodecParameters();
    avcodec_parameters_copy(codecpar.get(), audio_stream->codecpar);

    audio_bus.in_stream = audio_stream;
    audio_bus.out_codecpar = codecpar;
    if (!audio_filter_->Init(audio_bus)) return false;
  }

  return true;
}

void FileMediaSrc::Start() {
  Loop();
}

void FileMediaSrc::Cancel() {
  stop_ = true;
}

void FileMediaSrc::Close() {
  LOG(VERBOSE) << "file_media_src:" << file_url_ << " close";
  if (video_filter_) video_filter_->Close();
  if (audio_filter_) audio_filter_->Close();
}

}
