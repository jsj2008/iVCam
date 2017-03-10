 
#ifndef _H264_DEC_IMPL_H_
#define _H264_DEC_IMPL_H_

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

#include "h264dec.h"

class H264DecImpl
{
public:

	int Open(H264DecParam& param);
	void Close();
    std::shared_ptr<DecodeFrame> Decode(unsigned char* data, unsigned int len, long long pts, long long dts);
    std::shared_ptr<DecodeFrame2> Decode2(unsigned char* data, unsigned int len, long long pts, long long dts);
    void FlushBuff();

private:
    
	unsigned int width_ = 0;
	unsigned int height_ = 0;
    AVPixelFormat pix_fmt_ = AV_PIX_FMT_NONE;

	AVCodec* codec_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
	SwsContext* sw_ctx_ = nullptr;
};

#endif

