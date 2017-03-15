#ifndef _THREAD_SAFE_QUEUE_H
#define _THREAD_SAFE_QUEUE_H

#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue
{
public:
  ThreadSafeQueue() = default;
  typedef typename std::deque<T>::size_type size_type;

  ThreadSafeQueue(const ThreadSafeQueue& other) {
      std::lock_guard<std::mutex> lk(other.data_queue_);
      data_queue_ = other.data_queue_;
  }

  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

  bool try_peek(T& value);

  void push(const T& value);

  bool empty() const;
  size_type size() const;

  bool try_pop(T& value);
  std::shared_ptr<T> try_pop();

  void clear();

  bool wait_and_pop(T& value, const std::chrono::milliseconds& timeout = std::chrono::milliseconds(30));
  std::shared_ptr<T> wait_and_pop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(30));

private:
  mutable std::mutex mtx_;
  std::queue<T> data_queue_;
  std::condition_variable data_cond_;
};

template<typename T>
inline bool ThreadSafeQueue<T>::try_peek(T &value) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (data_queue_.empty()) return false;
  value = data_queue_.front();
  return true;
}

template<typename T>
inline bool ThreadSafeQueue<T>::try_pop(T & value) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (data_queue_.empty()) return false;
  value = data_queue_.front();
  data_queue_.pop();
  return true;
}

template<typename T>
inline void ThreadSafeQueue<T>::push(const T& value) {
  std::lock_guard<std::mutex> lk(mtx_);
  data_queue_.push(value);
  data_cond_.notify_one();
}

template<typename T>
inline bool ThreadSafeQueue<T>::empty() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return data_queue_.empty();
}

template<typename T>
inline typename ThreadSafeQueue<T>::size_type ThreadSafeQueue<T>::size() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return data_queue_.size();
}

template<typename T>
inline std::shared_ptr<T> ThreadSafeQueue<T>::try_pop() {
  std::lock_guard<std::mutex> lk(mtx_);
  if (data_queue_.empty()) return std::shared_ptr<T>();
  std::shared_ptr<T> value = std::make_shared<T>(data_queue_.front());
  data_queue_.pop();
  return value;
}

template<typename T>
inline bool ThreadSafeQueue<T>::wait_and_pop(T & value, const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lck(mtx_);
  bool ret = data_cond_.wait_for(lck, timeout, [this]()->bool {
    return !data_queue_.empty();
  });
  if (ret) {
      value = data_queue_.front();
      data_queue_.pop_front();
      return true;
  } else {
      return false;
  }
}

template<typename T>
inline std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop(const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lck(mtx_);
  bool ret = data_cond_.wait_for(lck, timeout, [this]()->bool {
    return !data_queue_.empty();
  });
  if (ret) {
      std::shared_ptr<T> res = std::make_shared<T>(data_queue_.front());
      data_queue_.pop();
      return res;
  } else {
      return std::shared_ptr<T>();
  }
}

template<typename T>
inline void ThreadSafeQueue<T>::clear() {
  std::lock_guard<std::mutex> lk(mtx_);
  std::queue<T> empty_queue;
  std::swap(data_queue_, empty_queue);
}
#endif
