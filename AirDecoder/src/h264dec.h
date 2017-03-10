
#ifndef _H264_DEC_H_
#define _H264_DEC_H_

#ifdef _WINDOWS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API 
#endif

#include "util.h"
#include <memory>

struct H264DecParam
{
	unsigned int width = 0;
	unsigned int height = 0;
    unsigned char pix_fmt = CM_PIX_FMT_YUV420P;
	unsigned char threads = 8;
	unsigned char* sps = nullptr;
	unsigned int sps_len = 0;
    unsigned char* pps = nullptr;
    unsigned int pps_len = 0;
};

class H264DecImpl;

class DLL_API H264Dec
{
public:
    
	~H264Dec();
	int Open(H264DecParam& param);
	void Close();
	std::shared_ptr<DecodeFrame> Decode(unsigned char* data, unsigned int len, long long pts, long long dts);
    std::shared_ptr<DecodeFrame2> Decode2(unsigned char* data, unsigned int len, long long pts, long long dts);
    void FlushBuff();

private:
	
	H264DecImpl* h264decimpl_ = nullptr;
};


#endif

