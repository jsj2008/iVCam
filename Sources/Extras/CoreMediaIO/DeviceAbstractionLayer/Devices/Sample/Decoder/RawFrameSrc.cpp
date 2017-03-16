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
    {}
    
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
            ret = mCamera->readFrame(&frame);
            if (ret != 0)
            {
                LOG(ERROR) << "Failed to read frame.";
                continue;
            }
            // Validate the frame.
            uint8_t* data = frame->data();
            if ((data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1)
                || (data[0] == 0 && data[1] == 0 && data[2] == 1))
            {
                // Calculate pts and dts
                // To be fixed
                
                // Create packet
                auto packet = NewH264Packet(frame->data(), frame->size(), 0, 0, false, mStreamIndex);

                if (video_filter_ && !video_filter_->Filter(packet))
                {
                    LOG(ERROR) << "video filter error.";
                    OnError();
                }
            }
            else
            {
                LOG(ERROR) << "Damaged frame will be dropped.";
                continue;
            }
        }
        LOG(VERBOSE) << "Loop finish.";
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
    
    bool RawFrameSrc::Prepare()
    {
        stop_ = false;
        eof_ = false;
        
        uint8_t spspps[512];
        int spspps_len;
        std::shared_ptr<Frame> frame;
        int ret;
        while (true)
        {
            ret = mCamera->readFrame(&frame);
            if (ret != 0)
            {
                LOG(ERROR) << "Failed to read frame.";
                continue;
            }
            
            // validate the frame
            uint8_t* data = frame->data();
            if ((data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1)
                || (data[0] == 0 && data[1] == 0 && data[2] == 1))
            {
                LOG(VERBOSE) << "Get extra data!";
                ParseSPSPPS(frame->data(), frame->size(), spspps, spspps_len);
                break;
            }
            else
            {
                continue;
            }
        }
        
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
            
            auto video_stream = NewH264Stream(mWidth, mHeight, frameRate, timeBase, AV_CODEC_ID_H264, AV_PIX_FMT_YUV422P, spspps, spspps_len);
            auto codecpar = NewAVCodecParameters();
            avcodec_parameters_copy(codecpar.get(), video_stream->codecpar);
            
            video_bus.in_stream = video_stream.get();
            video_bus.out_codecpar = codecpar;
            mStreamIndex = video_stream->index;
            
            if (!video_filter_->Init(video_bus)) return false;
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
        if (video_filter_) video_filter_->Close();
    }
    
    void RawFrameSrc::ParseNAL(const uint8_t* data, const int32_t data_size, unsigned char nal_type, int& start_pos, int& size)
    {
        start_pos = -1;
        size = 0;
        for (int i = 0; i < data_size - 4; i++)
        {
            int offset = 0;
            if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1)
            {
                offset = 4;
            }
            else if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
            {
                offset = 3;
            }
            else
            {
                continue;
            }
            
            if (start_pos != -1)
            {
                size = i - start_pos;
                break;
            }
            else
            {
                if (nal_type == (data[i + offset] & 0x1f))
                {
                    start_pos = i + offset;
                }
            }
            i += offset;
        }
    }
    
    bool RawFrameSrc::ParseSPSPPS(const uint8_t* data, const int32_t data_size, unsigned char* buffer, int& len)
    {
        int sps_start_pos;
        int sps_size;
        int pps_start_pos;
        int pps_size;
        
        ParseNAL(data, data_size, 7, sps_start_pos, sps_size);
        ParseNAL(data, data_size, 8, pps_start_pos, pps_size);
        if (sps_size == 0 || pps_size == 0)
        {
            LOG(ERROR) << "sps_size == 0 or pps_size == 0";
            return false;
        }
        const int SPS_PPS_PAD = 0x0001;
        
        memcpy(buffer, &SPS_PPS_PAD, sizeof(int));
        memcpy(buffer + sizeof(int), data + sps_start_pos, sps_size);
        memcpy(buffer + sizeof(int) + sps_size, &SPS_PPS_PAD, sizeof(int));
        memcpy(buffer + sizeof(int) + sps_size + sizeof(int), data + pps_start_pos, pps_size);
        
        return true;
    }
    
}
