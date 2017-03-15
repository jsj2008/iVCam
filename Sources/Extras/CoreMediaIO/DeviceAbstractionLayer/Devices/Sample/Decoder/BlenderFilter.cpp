//
//  BlenderFilter.cpp
//  AirDecoder
//
//  Created by zhangzhongke on 3/13/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#include "BlenderFilter.hpp"

namespace ins {
    
    BlenderFilter::BlenderFilter(unsigned int input_width, unsigned int input_height, unsigned int output_width, unsigned int output_height, std::string offset)
    {
        mParams.input_width = input_width;
        mParams.input_height = input_height;
        mParams.output_width = output_width;
        mParams.output_height = output_height;
        mParams.offset = offset;
        mBlender = new CBlenderWrapper;
        mBlendedFrame = NewAVFrame(mParams.output_width, mParams.output_height, AV_PIX_FMT_RGBA);
    }
    
    bool BlenderFilter::Init(MediaBus &bus)
    {
        if (mBlender) {
            mBlender->capabilityAssessment();
            mBlender->getSingleInstance(CBlenderWrapper::FOUR_CHANNELS);
            mBlender->initializeDevice();
        }
        
        bus.out_codecpar->width = mParams.output_width;
        bus.out_codecpar->height = mParams.output_height;
        bus.out_codecpar->format = AV_PIX_FMT_RGBA;
        return this->next_filter_->Init(bus);
    }
    
    bool BlenderFilter::Filter(const sp<AVFrame> &frame)
    {
        if (mBlender && mBlendedFrame != nullptr) {
            mParams.input_data = (unsigned char*)frame->data;
            mParams.output_data = (unsigned char*)mBlendedFrame->data;
            mBlender->runImageBlender(mParams, CBlenderWrapper::PANORAMIC_BLENDER);
            return next_filter_->Filter(mBlendedFrame);
        }
        
        return next_filter_->Filter(frame);
    }
    
    void BlenderFilter::Close()
    {
        delete mBlender;
        return next_filter_->Close();
    }
    
}
