//
// Created by jerett on 16/12/2.
//

#ifndef INSMEDIAUTIL_DECODER_H
#define INSMEDIAUTIL_DECODER_H

#include "sp.h"

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace ins {

class Decoder {
  //using OnDecodeFrameFilter = std::function<sp<AVFrame>(const sp<AVFrame>&)>;
public:
  explicit Decoder(const sp<AVCodecContext> &dec_ctx) noexcept;
  ~Decoder();
  /////
  ///// \param filter some hardware codec may need filter to return suitable frame type
  //void RegisterDecodeFilter(const OnDecodeFrameFilter &filter) {
  //  filter_ = filter;
  //}

  sp<const AVCodecContext> dec_ctx() const {
    return dec_ctx_;
  }

  int Open();
  int SendPacket(const sp<AVPacket> &in_pkt);
  int SendEOF();
  int ReceiveFrame(sp<AVFrame> &out_frame);
  ///reset decoder.
  void FlushBuffer();

private:
  sp<AVCodecContext> dec_ctx_;
  //OnDecodeFrameFilter filter_;
};


}


#endif //INSMEDIAUTIL_DECODER_H
