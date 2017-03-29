#include "decode_filter.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}
#include <llog/llog.h>
#include <av_toolbox/any.h>
#include <av_toolbox/ffmpeg_util.h>

void ins::DecodeFilter::SetOpt(const std::string & key, const any & val) {
  if (key == kUseHwaccelDecode) {
    enable_hwaccel_ = any_cast<bool>(val);
  }
}

bool ins::DecodeFilter::CreateHardwareDecoder(const AVStream *stream) {
  CHECK(stream != nullptr);
  dec_ctx_ = NewAVHwaccelDecodeContextFromID(stream->codecpar->codec_id);
  if (dec_ctx_) {
    //cpy fmt and reset this is for videotoolbox decode.
    auto fmt_cpy = dec_ctx_->pix_fmt;
    avcodec_parameters_to_context(dec_ctx_.get(), stream->codecpar);
    dec_ctx_->pix_fmt = fmt_cpy;
    dec_ctx_->refcounted_frames = 1;
    decoder_.reset(new Decoder(dec_ctx_));
    return decoder_->Open() == 0;
  } else {
    LOG(VERBOSE) << "find Hwaccel failed.";
    return false;
  }
}

bool ins::DecodeFilter::CreateSoftwareDecoder(const AVStream *stream) {
  CHECK(stream != nullptr);
  enable_hwaccel_ = false;
  dec_ctx_ = NewAVDecodeContextFromID(stream->codecpar->codec_id);
  LOG(VERBOSE) << "change to decoder:" << avcodec_get_name(stream->codecpar->codec_id);
  dec_ctx_->refcounted_frames = 1;
  CHECK(avcodec_parameters_to_context(dec_ctx_.get(), stream->codecpar) == 0);
  decoder_.reset(new Decoder(dec_ctx_));
  return decoder_->Open() == 0;
}

bool ins::DecodeFilter::Init(MediaBus &bus) {
  CHECK(next_filter_ != nullptr);
  time_base_ = bus.in_stream->time_base;
  //first atemp use hardware decoder, switch to CPU decode otherwise
  if ((enable_hwaccel_ && CreateHardwareDecoder(bus.in_stream)) ||
      CreateSoftwareDecoder(bus.in_stream)) {
    CHECK(avcodec_parameters_from_context(bus.out_codecpar.get(), dec_ctx_.get()) >= 0);
    return next_filter_->Init(bus);
  } else {
    return false;
  }
}

void ins::DecodeFilter::Close() {
  decoder_->SendEOF();
  sp<AVFrame> picture;
  while (decoder_->ReceiveFrame(picture) != AVERROR_EOF) {
    //LOG(VERBOSE) << "flush frame size:" << picture->width << " " << picture->height
    //  << " time:" << TimestampToMs(picture->pts, time_base_);
    if (!next_filter_->Filter(picture)) break;
  }
  next_filter_->Close();
}

bool ins::DecodeFilter::Filter(const sp<ARVPacket> &pkt) {
  auto ret = decoder_->SendPacket(pkt);
  sp<AVFrame> picture;
  ret = decoder_->ReceiveFrame(picture);
  if (ret == 0) {
    if (pkt->data)
    {
      delete [] pkt->data;
      pkt->data = nullptr;
    }
    return next_filter_->Filter(picture);
  }
  return true;
}
