// Implementation of the circular buffer filter

// Copyright (c) 2017 insta360, jertt
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <av_toolbox/any.h>
#include <llog/llog.h>
#include <av_toolbox/dynamic_circular_buffer.h>
#include "media_filter.h"

namespace ins {

const std::string kCircularBufferCapactiy("capacity"); // value type:int, set circular buffer capacity.

template <typename DataType>
class CircularBufferFilter : public MediaFilter<DataType, DataType> {
public:
  CircularBufferFilter(int capacity) : circular_buffer_(capacity) {
    ;
  }
  ~CircularBufferFilter() = default;

  void SetOpt(const std::string &key, const any &value) override;
  bool Init(MediaBus &bus) override;
  bool Filter(const DataType &data) override;
  void Close() override;

private:
  void UnqueueBuffer();

private:
  bool to_release_ = false;
  std::atomic_bool err_ = {false};
  dynamic_circular_buffer<DataType> circular_buffer_;
  std::mutex mtx_;
  std::thread run_thread_;
  std::condition_variable unqueue_condition_;
};

template <typename DataType>
void CircularBufferFilter<DataType>::SetOpt(const std::string &key, const any &value) {
  if (key == kCircularBufferCapactiy) {
    auto capacity = any_cast<int>(value);
    circular_buffer_.set_capacity(capacity);
  }
}

template<typename DataType>
bool CircularBufferFilter<DataType>::Init(MediaBus &bus) {
  CHECK(this->next_filter_ != nullptr);
  if (!this->next_filter_->Init(bus)) {
    return false;
  } else {
    to_release_ = false;
    err_ = false;
    run_thread_ = std::thread(&CircularBufferFilter<DataType>::UnqueueBuffer, this);
    return true;
  }
}

template<typename DataType>
bool CircularBufferFilter<DataType>::Filter(const DataType &data) {
  if (err_) return false;
  std::unique_lock<std::mutex> lck(mtx_);
  circular_buffer_.push_back(data);
  unqueue_condition_.notify_one();
  return true;
}

template<typename DataType>
void CircularBufferFilter<DataType>::Close() {
  if (!to_release_) {
    to_release_ = true;
    unqueue_condition_.notify_all();
    if (run_thread_.joinable()) {
      run_thread_.join();
    }
    this->next_filter_->Close();
    LOG(VERBOSE) << "~CircularBuffer close.";
  }
}

template<typename DataType>
void CircularBufferFilter<DataType>::UnqueueBuffer() {
  while (!to_release_) {
    std::unique_lock<std::mutex> lck(mtx_);
    unqueue_condition_.wait(lck, [this]() {
      return !circular_buffer_.empty() || to_release_;
    });
    if (to_release_) break;
    auto data = circular_buffer_.front();
    circular_buffer_.pop_front();
    lck.unlock();

    //LOG(INFO) << "to filter:" << buffer_queue_.size();
    auto ret = this->next_filter_->Filter(data);
    if (!ret) err_ = true;
  }
  while (!err_ && !circular_buffer_.empty()) {
    auto &data = circular_buffer_.front();
    if (!this->next_filter_->Filter(data)) err_ = true; //break the loop
    circular_buffer_.pop_front();
  }
  LOG(INFO) << "exit unqueue thread...";
}


}



