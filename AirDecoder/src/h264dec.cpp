
#include "h264dec.h"
#include "h264decimpl.h"


H264Dec::~H264Dec()
{
	Close();
}

int H264Dec::Open(H264DecParam& param)
{
	h264decimpl_ = new H264DecImpl;
		
	return h264decimpl_->Open(param);
}

void H264Dec::Close()
{
	if (h264decimpl_)
	{
		h264decimpl_->Close();
		delete h264decimpl_;
		h264decimpl_ = nullptr;
	}
}

std::shared_ptr<DecodeFrame> H264Dec::Decode(unsigned char* data, unsigned int len, long long pts, long long dts)
{
	if (h264decimpl_)
	{
		return h264decimpl_->Decode(data, len, pts, dts);
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<DecodeFrame2> H264Dec::Decode2(unsigned char* data, unsigned int len, long long pts, long long dts)
{
    if (h264decimpl_)
    {
        return h264decimpl_->Decode2(data, len, pts, dts);
    }
    else
    {
        return nullptr;
    }
}


void H264Dec::FlushBuff()
{
    if (h264decimpl_)
    {
        h264decimpl_->FlushBuff();
    }
}

