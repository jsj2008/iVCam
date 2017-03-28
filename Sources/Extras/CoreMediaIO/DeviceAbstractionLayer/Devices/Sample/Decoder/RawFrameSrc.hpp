//
//  RawFrameSrc.hpp
//  AirDecoder
//
//  Created by zhangzhongke on 3/13/17.
//  Copyright © 2017 Insta360. All rights reserved.
//

#ifndef RawFrameSrc_hpp
#define RawFrameSrc_hpp

#include <string>
#include <tuple>
#include <av_toolbox/sp.h>
#include <editor/media_src.h>
#include "AtomCamera.h"
#include "Frame.h"

struct AVStream;

namespace ins {
    
    class any;
    
    class RawFrameSrc : public MediaSrc {
    public:
        
        explicit RawFrameSrc(std::shared_ptr<AtomCamera> camera, int width, int height);
        ~RawFrameSrc();
        RawFrameSrc(const RawFrameSrc&) = delete;
        RawFrameSrc(RawFrameSrc &&) = delete;
        RawFrameSrc& operator=(const RawFrameSrc&) = delete;
        
        const AVStream* video_stream() const; 
        
        void SetOpt(const std::string &key, const any &val);
        bool Prepare() override;
        void Start() override;
        void Cancel() override;
        void Close() override;
        
    public:
        double progress() const override {
            return progress_;
        }
        
        bool eof() const override {
            return eof_;
        }
        
    private:
        void OnEnd();
        void OnError();
        void Loop();
        sp<ARVPacket> NewMJpegPacket(const uint8_t * data, int size, bool keyframe, int stream_index);
        
    private:
        bool eof_ = false;
        double progress_ = 0;
        bool stop_ = false;
        std::shared_ptr<AtomCamera> mCamera;
        int mStreamIndex;
        int mWidth;
        int mHeight;
    };
    
}

#endif /* RawFrameSrc_hpp */
