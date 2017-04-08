//
// Created by jerett on 16/7/14.
//

#ifndef INSPLAYER_MEDIA_PIPE_H
#define INSPLAYER_MEDIA_PIPE_H

#include <vector>
#include <future>
#include <algorithm>
#include <numeric>
#include <thread>
#include <vector>
#include <llog/llog.h>
#include "file_media_src.h"

namespace ins {

class MediaPipe {
public:
  using SharedSource = std::shared_ptr<ins::MediaSrc>;
  enum MediaPipeState {
    kMediaPipeEnd,
    kMediaPipeError
  };
  using STATE_CALLBACK = std::function<void(MediaPipeState state)>;

public:
  MediaPipe() noexcept  = default;
  ~MediaPipe() {
    Wait();
    LOG(VERBOSE) << "~MediaPipe";
  }

  /// \param source add a media src
  void AddMediaSrc(const SharedSource &source) {
    source->RegisterCallback([this](MediaSrc::MediaSrcState state) {
      if (state == MediaSrc::MediaSourceEnd) {

        //check is all eof
        for (auto &src : sources_) {
          if (!src->eof()) {
            return;
          }
        }
        OnEnd();
      } else if (state == MediaSrc::MediaSourceError) {
        OnError();
      }
    });
    sources_.push_back(source);
  }

  void RegisterCallback(const STATE_CALLBACK &state_callback) {
    state_callback_ = state_callback;
  }

  double progress() const {
    auto min_ele_itr = std::min_element(std::begin(sources_), std::end(sources_),
                                        [](const SharedSource& lhs, const SharedSource& rhs)->bool {
      return lhs->progress() < rhs->progress();
    });
    if (min_ele_itr == std::end(sources_)) return 0;
    return (*min_ele_itr)->progress();
  }
  
  void Run() {
    for (auto &ws : sources_) {
      std::thread run_thread([&](){
        ws->Start();
      });
      src_threads_.push_back(std::move(run_thread));
    }
  }

  void Wait() {
    for (auto &thread : src_threads_) {
      if (thread.joinable()) thread.join();
    }
    OnEnd();
  }

  void Cancel() {
    for (auto &src : sources_) {
      src->Cancel();
    }
  }

  void OnEnd() {
    Release();
    if (state_callback_) state_callback_(kMediaPipeEnd);
  }

  void OnError() {
    Cancel();
    Release();
    if (state_callback_) state_callback_(kMediaPipeError);
  }
  
private:
  void Release() {
    for (auto &src : sources_) {
      src->Close();
    }
  }


private:
  std::vector<std::thread> src_threads_;
  std::vector<SharedSource> sources_;
  STATE_CALLBACK state_callback_;
};

}


#endif //INSPLAYER_MEDIA_PIPE_H
