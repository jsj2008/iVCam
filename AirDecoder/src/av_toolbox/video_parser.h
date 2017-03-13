#pragma once

#include <string>
#include "sp.h"

struct AVFrame;

namespace ins {

class Demuxer;

class VideoParser {
  
public:
  explicit VideoParser(const std::string &url);
  bool Open();
  /**
   * \brief Screen shot video at target time in ms.
   * \param position_ms seek time in ms
   * \param img screen shot img. img->pts will rescale in ms. pix fmt is raw.
   * \return result code. 0 on success, -1 on no video stream, -2 on seek failed, -3 on decode failed.
   */
  int ScreenshotAt(int64_t position_ms, sp<AVFrame> &img);

private:
  std::string url_;
  sp<Demuxer> demuxer_;
};

}

