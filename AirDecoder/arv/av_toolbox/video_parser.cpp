#include "video_parser.h"

#include "demuxer.h"
#include "decoder.h"
#include "ffmpeg_util.h"
#include <llog/llog.h>

namespace ins {

VideoParser::VideoParser(const std::string& url) : url_(url) {
  ;
}

bool VideoParser::Open() {
  demuxer_ = std::make_shared<Demuxer>(url_);
  return demuxer_->Open() == 0;
}

int VideoParser::ScreenshotAt(int64_t position_ms, sp<AVFrame>& img) {
  auto video_stream = demuxer_->video_stream();
  if (video_stream == nullptr) {
    return -1;
  }

  {
    //demux
    auto seek_dts = MsToTimestamp(position_ms, video_stream->time_base);
    if (demuxer_->Seek(video_stream->index, seek_dts, AVSEEK_FLAG_BACKWARD) < 0) {
      return -2;
    }
  }

  {
    //init decoder
    auto decode_ctx = NewAVDecodeContextFromID(video_stream->codecpar->codec_id);
    decode_ctx->refcounted_frames = 1;
    CHECK(avcodec_parameters_to_context(decode_ctx.get(), video_stream->codecpar) == 0);
    Decoder decoder(decode_ctx);
    if (decoder.Open() != 0) return -3;
    sp<ARVPacket> video_pkt;
    while (demuxer_->NextPacket(video_pkt) == 0 && video_pkt->stream_index != video_stream->index) { ; }
    //decode
    if (video_pkt && video_pkt->stream_index == video_stream->index) {
      decoder.SendPacket(video_pkt);
      decoder.SendEOF();
      if (decoder.ReceiveFrame(img) == 0) {
        img->pts = TimestampToMs(img->pts, video_stream->time_base);
        return 0;
      } else {
        return -3;
      }
    } else {
      return -3;
    }
  }

  
}

}
