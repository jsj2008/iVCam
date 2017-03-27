//
//  BlenderSink.hpp
//  Sample
//
//  Created by zhangzhongke on 3/16/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#ifndef BlenderSink_hpp
#define BlenderSink_hpp

extern "C" {
#include <libavformat/avformat.h> 
}
 
#include <editor/filter/media_filter.h> 
#include <llog/llog.h>

namespace ins{
    
    class BlenderSink: public MediaSink<sp<AVFrame>> {
    public:
        explicit BlenderSink(uint8_t* queue);
        ~BlenderSink() = default;
        
        bool Prepare() override;
        bool Init(MediaBus &bus) override;
        void SetOpt(const std::string &key, const any &value) override;
        bool Filter(const sp<AVFrame> &frame) override;
        void Close() override;
        
    private:
        uint8_t* mBuffer;
    };
    
}

#endif /* BlenderSink_hpp */
