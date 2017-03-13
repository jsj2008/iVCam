//
// Created by jerett on 16/7/14.
//

#include "mux_sink.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
#include <av_toolbox/ffmpeg_util.h>
#include <av_toolbox/any.h>

using namespace std::chrono;

namespace ins {

MuxSink::MuxSink(const std::string &format, const std::string &file_url) : file_url_(file_url) {
  auto out_fmt = NewOutputAVFormatContext(format, file_url);
  muxer_.reset(new Muxer(out_fmt));
}

void MuxSink::SetOpt(const std::string &key, const any &value) {
  if (key == kMovFlag) {
    auto val = any_cast<std::string>(value);
    if (val == kMovFlag_FragmentMp4) {
      fragment_mp4_ = true;
    }
    auto ctx = muxer_->mux_ctx();
    ctx->oformat->flags |= AVFMT_GLOBALHEADER;
    ctx->oformat->flags |= AVFMT_ALLOW_FLUSH;
    av_opt_set(ctx->priv_data, key.c_str(), val.c_str(), 0);
  }

  if (key == kFragmentFrameCount) {
    auto cnt = any_cast<int>(value);
    fragment_split_cnt_ = cnt;
    CHECK(fragment_split_cnt_ > 0);
  }

  if (key == kSphericalProjection) {
    spherical_type_ = any_cast<std::string>(value);
  }
  if (key == kStereo3DType) {
    stereo_type_ = any_cast<std::string>(value);
  }
}

bool MuxSink::Init(MediaBus &bus) {
  sp<AVStream> stream;
  if (bus.in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    stream = muxer_->GetVideoStream();

    if (!spherical_type_.empty() && !stereo_type_.empty()) {
      av_dict_set(&stream->metadata, "spherical_mapping", spherical_type_.c_str(), 0);
      av_dict_set(&stream->metadata, "stereo_3d", stereo_type_.c_str(), 0);
    }

    src_video_timebase_ = bus.in_stream->time_base;
  } else if (bus.in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    stream = muxer_->GetAudioStream();
    src_audio_timebase_ = bus.in_stream->time_base;
  } else {
    LOG(WARNING) << "codec type unknown:" << bus.in_stream->codecpar->codec_type;
    return false;
  }
  avcodec_parameters_copy(stream->codecpar, bus.out_codecpar.get());
  stream->codecpar->codec_tag = 0;
  return true;
}

bool MuxSink::Prepare() {
  fps_timer_.Reset();
  return muxer_->Open() == 0;
}

bool MuxSink::Filter(const sp<ARVPacket> &pkt) {
  if (pkt->media_type == AVMEDIA_TYPE_VIDEO) {
    {
      //handle fragment if set
      if (fragment_mp4_ && ++fragment_counter_ % fragment_split_cnt_ == 0) {
        auto ctx = muxer_->mux_ctx();
        av_write_frame(ctx.get(), nullptr);
        fragment_counter_ = 0;
        LOG(VERBOSE) << "new fragment.";
      }
    }

    {
      //fps statistic
      ++log_counter_;
      auto pass_ms = fps_timer_.Pass();
      if (pass_ms >= 3000) {
        auto fps = log_counter_ * 1000 / pass_ms;
        fps_timer_.Reset();
        log_counter_ = 0;
        LOG(VERBOSE) << "fps:" << fps;
      }
    }
    return muxer_->WriteVideoPacket(pkt, src_video_timebase_) == 0;
  } else if (pkt->media_type == AVMEDIA_TYPE_AUDIO) {
    auto out_audio_stream = muxer_->GetAudioStream();
    return muxer_->WriteAudioPacket(pkt, src_audio_timebase_) == 0;
  } else {
    LOG(WARNING) << "mux_sink: unknown type";
  }
  return true;
}

void MuxSink::Close() {
  muxer_->Close();
}

}
