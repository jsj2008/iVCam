//
//  BlenderFilter.hpp
//  AirDecoder
//
//  Created by zhangzhongke on 3/13/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#ifndef BlenderFilter_hpp
#define BlenderFilter_hpp

#include <editor/filter/media_filter.h> 
#include "../BlenderWrapper.h"
#include <av_toolbox/ffmpeg_util.h>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
}

namespace ins {
    
    class BlenderFilter : public VideoImageFilter<sp<AVFrame>> {
    public:
        BlenderFilter(unsigned int input_width, unsigned int input_height, unsigned int output_width, unsigned int output_height, std::string offset);
        ~BlenderFilter() = default;
        
        bool Init(MediaBus &bus) override;
        bool Filter(const sp<AVFrame> &frame) override;
        void Close() override;
        
    private:
        CBlenderWrapper* mBlender; 
        sp<AVFrame> mBlendedFrame;
        BlenderParams mParams;
    };
    
}

#endif /* BlenderFilter_hpp */
