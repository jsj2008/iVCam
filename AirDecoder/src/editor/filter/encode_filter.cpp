//
// Created by jerett on 16/8/11.
//

#include "encode_filter.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#if (__APPLE__)
#include <libavcodec/videotoolbox.h>
#endif
#include <libavutil/opt.h>
};
#include <av_toolbox/any.h>
#include <av_toolbox/ffmpeg_util.h>

namespace ins {

void EncodeFilter::SetOpt(const std::string &key, const any &value) {
  if (key == kEncodeBitrate) {
    bitrate_ = any_cast<int>(value);
    LOG(VERBOSE) << "set encode bitrate:" << bitrate_;
  }

  if (key == kEncodeBFrames) {
    //enc_ctx_->max_b_frames = any_cast<int>(value);
    bframes_ = any_cast<int>(value);
    LOG(VERBOSE) << "set encode bframes:" << bframes_;
  }

  if (key == kX264EncodePreset) {
    preset_ = any_cast<std::string>(value);
    LOG(VERBOSE) << "set encode preset:" << preset_;
  }

  if (key == kX264EncodeTune) {
    tune_ = any_cast<std::string>(value);
    LOG(VERBOSE) << "set encode tune:" << tune_;
  }

  if (key == kEncodeThreads) {
    threads_ = any_cast<int>(value);
    LOG(VERBOSE) << "set encode threads:" << threads_;
  }

  if (key == kUseHwaccelEncode) {
    enable_hwaccel_ = any_cast<bool>(value);
    LOG(VERBOSE) << "set enable_hwaccel:" << enable_hwaccel_;
  }
}

bool EncodeFilter::CreateHwaccelEncoder(const MediaBus &bus) {
  enc_ctx_ = NewAVHwaccelEncodeContextFromID(encodec_id_);
  if (enc_ctx_) {
    LOG(VERBOSE) << "to use encoder:" << enc_ctx_->codec->name;
    return ConfigureEncContext(bus);
  } else {
    return false;
  }
}

bool EncodeFilter::CreateSoftwareEncoder(const MediaBus &bus) {
  enable_hwaccel_ = false;
  enc_ctx_ = NewAVEncodeContextFromID(encodec_id_);
  if (enc_ctx_) {
    LOG(VERBOSE) << "switch to use encoder:" << enc_ctx_->codec->name;
    return ConfigureEncContext(bus);
  } else {
    return false;
  }
}

bool EncodeFilter::ConfigureEncContext(const MediaBus &bus) {
  enc_ctx_->width = bus.out_codecpar->width;
  enc_ctx_->height = bus.out_codecpar->height;
  enc_ctx_->pix_fmt = (AVPixelFormat)bus.out_codecpar->format;
  enc_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  enc_ctx_->time_base = bus.in_stream->time_base;
  if (bitrate_ != -1) enc_ctx_->bit_rate = bitrate_;
  if (bframes_ != -1) enc_ctx_->max_b_frames = bframes_;
  if (threads_ != -1) enc_ctx_->thread_count = threads_;
  if (gop_size_ != -1) enc_ctx_->gop_size = gop_size_;

  bool ffmpeg_videotoolbox_bug = (std::strcmp(enc_ctx_->codec->name, "h264_videotoolbox") == 0 &&
                                  bus.out_codecpar->format == AV_PIX_FMT_VIDEOTOOLBOX);

  if (std::strcmp(enc_ctx_->codec->name, "libx264") == 0) {
    if (!preset_.empty()) av_opt_set(enc_ctx_->priv_data, "preset", preset_.c_str(), 0);
    if (!tune_.empty()) av_opt_set(enc_ctx_->priv_data, "tune", tune_.c_str(), 0);
    if (bitrate_ != -1) av_opt_set(enc_ctx_->priv_data, "x264-params", "force-cfr=1", 0);
    if (bus.out_codecpar->format == AV_PIX_FMT_VIDEOTOOLBOX) {
      enc_ctx_->pix_fmt = AV_PIX_FMT_NV12;
    }
    auto frame_rate = bus.in_stream->avg_frame_rate;
    enc_ctx_->time_base = { frame_rate.den, frame_rate.num };
  } else if (ffmpeg_videotoolbox_bug) {
    //绕过ffmpeg的生成extradata的bug
    enc_ctx_->pix_fmt = AV_PIX_FMT_NV12;
  }
  
  encoder_.reset(new Encoder(enc_ctx_));
  if (encoder_->Open() != 0) {
    return false;
  }
  if (ffmpeg_videotoolbox_bug) {
    enc_ctx_->pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
  }
  return true;
}

bool EncodeFilter::Init(MediaBus &bus) {
  stream_index_ = bus.in_stream->index;
  if ((enable_hwaccel_ && CreateHwaccelEncoder(bus)) || CreateSoftwareEncoder(bus)) {
    CHECK(avcodec_parameters_from_context(bus.out_codecpar.get(), enc_ctx_.get()) == 0);
    return next_filter_->Init(bus);
  } else {
    return false;
  }
}

bool EncodeFilter::Filter(const sp<AVFrame> &frame) {
  if (std::strcmp(enc_ctx_->codec->name, "libx264") == 0 && frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
#if (__APPLE__)
    CVPixelBufferRef pixbuf = (CVPixelBufferRef)frame->data[3];
    auto nv12_frame = CopyCVPixelBufferToFrame(pixbuf);
    av_frame_copy_props(nv12_frame.get(), frame.get());
    encoder_->SendFrame(nv12_frame);
#else
    LOG(ERROR) << "how strange here.";
    exit(EXIT_FAILURE);
#endif
  } else {
    encoder_->SendFrame(frame);
  }
  sp<ARVPacket> pkt;
  auto ret = encoder_->ReceivePacket(pkt);
  if (ret == 0) {
    //LOG(VERBOSE) << "encode pkt pts:" << pkt->pts;
    pkt->stream_index = stream_index_;
    return next_filter_->Filter(pkt);
  }
  return true;
}

void EncodeFilter::Close() {
  encoder_->SendEOF();
  sp<ARVPacket> pkt;
  while (encoder_->ReceivePacket(pkt) == 0) {
    pkt->stream_index = stream_index_;
    if (!next_filter_->Filter(pkt)) break;
  }
  next_filter_->Close();
}

}
