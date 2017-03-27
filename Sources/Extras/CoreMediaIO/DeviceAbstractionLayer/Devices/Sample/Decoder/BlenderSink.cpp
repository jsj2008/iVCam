//
//  BlenderSink.cpp
//  Sample
//
//  Created by zhangzhongke on 3/16/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#include "BlenderSink.hpp"

namespace ins {
    
    BlenderSink::BlenderSink(boost::lockfree::spsc_queue<std::shared_ptr<AVFrame>, boost::lockfree::capacity<5> >* queue)
    : mQueue(queue)
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
        if (mQueue->write_available()) {
            mQueue->push(frame); 
        }
        else
        {
            LOG(ERROR) << "The frame buffer is full.";
        }
        return true;
    }
    
    void BlenderSink::Close()
    {
        LOG(VERBOSE) << "The frame sink closed.";
    }
    
}
