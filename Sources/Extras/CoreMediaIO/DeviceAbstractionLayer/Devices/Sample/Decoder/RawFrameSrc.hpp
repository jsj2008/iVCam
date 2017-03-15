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
#include "media_src.h"

struct AVStream;

namespace ins {
    
    const static std::string kSrcMode("read_mode");
    class any;
    class Demuxer;
    
    class FileMediaSrc : public MediaSrc {
    public:
        ///
        /// \param url media source URL
        explicit FileMediaSrc(const std::string &url);
        ~FileMediaSrc();
        FileMediaSrc(const FileMediaSrc&) = delete;
        FileMediaSrc(FileMediaSrc &&) = delete;
        FileMediaSrc& operator=(const FileMediaSrc&) = delete;
        
        /**
         * @param start_ms Start time in ms
         * @param end_ms   End time in ms
         */
        void set_trim_range(int64_t start_ms, int64_t end_ms) noexcept;
        
        const AVStream* video_stream() const;
        const AVStream* audio_stream() const;
        int64_t video_duration() const;
        int64_t audio_duration() const;
        
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
        bool realtime_ = false;
        std::string file_url_;
        std::tuple<bool, int64_t , int64_t> range_ = {false, 0, 0};
        sp<Demuxer> demuxer_;
    };
    
}

#endif /* RawFrameSrc_hpp */
