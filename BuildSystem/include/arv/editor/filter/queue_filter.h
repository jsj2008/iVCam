//
// Created by jerett on 16/7/24.
//

#ifndef INSPLAYER_QUEUE_FILTER_H
#define INSPLAYER_QUEUE_FILTER_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <av_toolbox/any.h>
#include "media_filter.h"


namespace ins {

const static std::string kQueueFilterMaxSize("queue_size"); //value type:int, default 30

template <typename DataType>
class QueueFilter : public MediaFilter<DataType, DataType> {
public:
  QueueFilter();
  ~QueueFilter();

  void SetOpt(const std::string &key, const any &value) override;
  bool Init(MediaBus &bus) override;
  bool Filter(const DataType &data) override;
  void Close() override;

private:
  void UnqueueBuffer();

private:
  /// set default queue max size 30
  int queue_max_size_ = 30;
  bool to_release_ = false;

  std::mutex queue_mtx_;
  std::atomic_bool err_ = {false};
  std::condition_variable unqueue_condition_;
  std::condition_variable queue_condition_;
  std::thread unqueue_thread_;
  std::queue<DataType> buffer_queue_;
};

template<typename DataType>
QueueFilter<DataType>::QueueFilter() = default;

template<typename DataType>
QueueFilter<DataType>::~QueueFilter() = default;

template<typename DataType>
void QueueFilter<DataType>::SetOpt(const std::string &key, const any &value) {
  if (key == kQueueFilterMaxSize) {
    queue_max_size_ = any_cast<int>(value);
    LOG(VERBOSE) << "queue config max size:" << queue_max_size_;
  }
}

template<typename DataType>
bool QueueFilter<DataType>::Init(MediaBus &bus) {
  CHECK(this->next_filter_ != nullptr);
  if (!this->next_filter_->Init(bus)) {
    return false;
  } else {
    to_release_ = false;
    err_ = false;
    unqueue_thread_ = std::thread(&QueueFilter::UnqueueBuffer, this);
    return true;
  }
}

template<typename DataType>
bool QueueFilter<DataType>::Filter(const DataType &data) {
  if (err_) return false;
  std::unique_lock<std::mutex> lck(queue_mtx_);
  if (queue_max_size_ > 0) {
    queue_condition_.wait(lck, [this]() {
      return buffer_queue_.size() < queue_max_size_;
    });
  }
  buffer_queue_.push(data);
  unqueue_condition_.notify_one();
  return true;
}

template<typename DataType>
void QueueFilter<DataType>::Close() {
  if (!to_release_) {
    to_release_ = true;
    unqueue_condition_.notify_all();
    if (unqueue_thread_.joinable()) {
      unqueue_thread_.join();
    }
    this->next_filter_->Close();
  }
  LOG(VERBOSE) << "QueueFilter Close.";
}

template <typename T>
void QueueFilter<T>::UnqueueBuffer() {
  while (!to_release_) {
    std::unique_lock<std::mutex> lck(queue_mtx_);
    unqueue_condition_.wait(lck, [this]() {
      return !buffer_queue_.empty() || to_release_;
    });
    if (to_release_) break;
    auto data = buffer_queue_.front();
    buffer_queue_.pop();
    lck.unlock();
    queue_condition_.notify_one();

//          LOG(INFO) << "to filter:" << buffer_queue_.size();
    auto ret = this->next_filter_->Filter(data);
    if (!ret) err_ = true;
  }
  while (!err_ && !buffer_queue_.empty()) {
    auto &data = buffer_queue_.front();
    if (!this->next_filter_->Filter(data)) err_ = true; //break the loop
    buffer_queue_.pop();
  }
  LOG(VERBOSE) << "unqueue thread finish.";
}


}


#endif //INSPLAYER_QUEUE_FILTER_H
