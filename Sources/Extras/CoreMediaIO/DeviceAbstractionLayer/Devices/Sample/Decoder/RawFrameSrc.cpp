//
//  RawFrameSrc.cpp
//  AirDecoder
//
//  Created by zhangzhongke on 3/13/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#include "RawFrameSrc.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
#include <llog/llog.h>
#include <chrono>
#include <algorithm>
#include <av_toolbox/ffmpeg_util.h>
#include <av_toolbox/any.h>

namespace ins {
    
    RawFrameSrc::RawFrameSrc(AtomCamera* camera, int width, int height)
    : mCamera(camera),
      mWidth(width),
      mHeight(height)
    {
    
    }
    
    RawFrameSrc::~RawFrameSrc()
    {
        
    }
    
    void RawFrameSrc::SetOpt(const std::string &key, const any &val)
    {
        
    }
    
    void RawFrameSrc::Loop()
    {
        int ret;
        std::shared_ptr<Frame> frame;
        
        while (!stop_)
        {
            if (mCamera->isClosed())
            {
                Cancel();
                continue;
            }
            ret = mCamera->readFrame(&frame);
            if (ret != 0)
            {
                LOG(ERROR) << "Failed to read frame.";
                continue;
            }
            // Create packet
            auto packet = NewMJpegPacket(frame->data(), (int)frame->size(), false, mStreamIndex);
            
            if (video_filter_ && !video_filter_->Filter(packet))
            {
                LOG(ERROR) << "video filter error.";
                OnError();
            }
        }
    }
    
    void RawFrameSrc::OnEnd()
    {
        eof_ = true;
        stop_ = true;
        if (state_callback_)
        {
            state_callback_(MediaSourceEnd);
        }
    }
    
    void RawFrameSrc::OnError()
    {
        stop_ = true;
        if (state_callback_)
        {
            state_callback_(MediaSourceError);
        }
    }
    
    sp<ARVPacket> RawFrameSrc::NewMJpegPacket(const uint8_t * data, int size, bool keyframe, int stream_index)
    {
        auto pkt = NewPacket();
        pkt->media_type = AVMEDIA_TYPE_VIDEO;
        CHECK(pkt != nullptr);
        pkt->data = new uint8_t[size];
        pkt->size = size;
        memcpy(pkt->data, data, size);
        pkt->flags = keyframe ? AV_PKT_FLAG_KEY : 0;
        pkt->stream_index = stream_index;
        return pkt;
    }

    
    bool RawFrameSrc::Prepare()
    {
        stop_ = false;
        eof_ = false;
        
        //init filter
        if (video_filter_)
        {
            MediaBus video_bus;
            AVRational frameRate;
            frameRate.num = 30;
            frameRate.den = 1;
            
            AVRational timeBase;
            timeBase.num = 1;
            timeBase.den = 1000;
            
            auto video_stream = NewVideoStream(mWidth, mHeight, frameRate, timeBase, AV_CODEC_ID_MJPEG, AV_PIX_FMT_ARGB);
            auto codecpar = NewAVCodecParameters();
            avcodec_parameters_copy(codecpar.get(), video_stream->codecpar);
            
            video_bus.in_stream = video_stream.get();
            video_bus.out_codecpar = codecpar;
            mStreamIndex = video_stream->index;
            
            if (!video_filter_->Init(video_bus))
            {
                LOG(ERROR) << "Failed to initialize all filters.";
                return false;
            }
        }
        
        return true;
    }
    
    void RawFrameSrc::Start()
    {
        Loop();
    }
    
    void RawFrameSrc::Cancel()
    {
        stop_ = true;
    }
    
    void RawFrameSrc::Close()
    {
        if (video_filter_)
        {
            video_filter_->Close();
        }
    }
    
}
