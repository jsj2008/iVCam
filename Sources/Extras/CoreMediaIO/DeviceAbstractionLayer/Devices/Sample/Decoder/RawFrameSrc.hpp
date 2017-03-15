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

struct AVStream;

namespace ins {
    
    class any;
    
    class RawFrameSrc : public MediaSrc {
    public:
        
        explicit RawFrameSrc();
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
        
    private:
        bool eof_ = false;
        double progress_ = 0;
        bool stop_ = false;
    };
    
}

#endif /* RawFrameSrc_hpp */
