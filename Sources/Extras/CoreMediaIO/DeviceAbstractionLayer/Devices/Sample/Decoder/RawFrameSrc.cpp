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
#include <av_toolbox/demuxer.h>
#include <av_toolbox/ffmpeg_util.h>
#include <av_toolbox/any.h>

namespace ins {
    
    RawFrameSrc::RawFrameSrc(){
        ;
    }
    
    RawFrameSrc::~RawFrameSrc() {
        
    }
    
    void RawFrameSrc::SetOpt(const std::string &key, const any &val) {
        
    }
    
    const AVStream* RawFrameSrc::video_stream() const {
        
    }
    
    void RawFrameSrc::Loop() {
        
        while (!stop_)
        {
            sp<ARVPacket> pkt; 
            // Create packet
            
            // Calculate pts and dts
//            audio_start_dts_ = av_rescale_q(video_start_dts_,
//                                            demuxer_->video_stream()->time_base,
//                                            demuxer_->audio_stream()->time_base);
//                     
//            pkt->pts -= video_start_dts_;
//            pkt->dts -= video_start_dts_;
            
            if (video_filter_ && !video_filter_->Filter(pkt))
            {
                LOG(ERROR) << "video filter err.";
                OnError();
            }
        }
        LOG(VERBOSE) << "loop finish .";
    }
    
    void RawFrameSrc::OnEnd() {
        LOG(VERBOSE) << "error eof...";
        eof_ = true;
        stop_ = true;
        if (state_callback_) {
            state_callback_(MediaSourceEnd);
        }
    }
    
    void RawFrameSrc::OnError() {
        stop_ = true;
        if (state_callback_) {
            state_callback_(MediaSourceError);
        }
    }
    
    bool RawFrameSrc::Prepare() {
    
        stop_ = false;
        eof_ = false;
        
        //init filter
        if (video_filter_) {
            MediaBus video_bus;
//            auto video_stream = NewH264Stream(0, 0, 0, 0, 0, 0, 0, 0);
//            auto codecpar = NewAVCodecParameters();
//            avcodec_parameters_copy(codecpar.get(), video_stream->codecpar);
//            
//            video_bus.in_stream = video_stream;
//            video_bus.out_codecpar = codecpar;
            if (!video_filter_->Init(video_bus)) return false;
        }
        
        return true;
    }
    
    void RawFrameSrc::Start() {
        Loop();
    }
    
    void RawFrameSrc::Cancel() {
        stop_ = true;
    }
    
    void RawFrameSrc::Close() {
        if (video_filter_) video_filter_->Close();
    }
    
}
