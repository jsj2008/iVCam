//
// Created by jerett on 16/7/14.
//

#ifndef INSPLAYER_MUX_SINK_H
#define INSPLAYER_MUX_SINK_H

extern "C" {
#include <libavutil/rational.h>
}
#include <av_toolbox/muxer.h>
#include <llog/llog.h> //use for Timer
#include "media_filter.h"

namespace ins {

const static std::string kMovFlag("movflags");
const static std::string kMovFlag_FragmentMp4("frag_custom+empty_moov");
const static std::string kFragmentFrameCount("fragment_frame_cnt");
const static std::string kSphericalProjection("spherical_type"); //val->string: equirectangular or cubemap(not support now)
const static std::string kStereo3DType("stereo_3d_type");//val->string: Must be mono, left-right, or top-bottom.

class MuxSink: public PacketSink {
public:
  explicit MuxSink(const std::string &format, const std::string &file_url);
  ~MuxSink() = default;

  bool Prepare() override;
  bool Init(MediaBus &bus) override;
  void SetOpt(const std::string &key, const any &value) override;
  bool Filter(const sp<ARVPacket> &pkt) override;
  void Close() override;

private:
  AVRational src_video_timebase_;
  AVRational src_audio_timebase_;
  bool fragment_mp4_ = false;
  int fragment_split_cnt_ = INT_MAX;
  int fragment_counter_ = 0;
  int log_counter_ = 0;
  Timer fps_timer_;
  std::string spherical_type_;
  std::string stereo_type_;

  std::string file_url_;
  up<Muxer> muxer_;
};

}


#endif //INSPLAYER_MUX_SINK_H
