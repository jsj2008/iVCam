//
//  BlenderSink.cpp
//  Sample
//
//  Created by zhangzhongke on 3/16/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#include "BlenderSink.hpp"

namespace ins {
    
    BlenderSink::BlenderSink(uint8_t* frame)
    : mBuffer(frame)
    {
        
    }
    
    bool BlenderSink::Prepare()
    {
        return true;
    }
    
    bool BlenderSink::Init(MediaBus &bus)
    {
        return true;
    }
    
    void BlenderSink::SetOpt(const std::string &key, const any &value)
    {
        
    }
    
    bool BlenderSink::Filter(const sp<AVFrame> &frame)
    {
        if (mBuffer)
        {
            memcpy(mBuffer, frame->data[0], frame->linesize[0]*frame->height); 
        }
        
        return true;
    }
    
    void BlenderSink::Close()
    {
        LOG(VERBOSE) << "The frame sink closed.";
    }
    
}
