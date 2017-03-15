// Implementation of the base circular buffer.

// Copyright (c) 2017 insta360, jertt
#pragma once
#include <deque>

namespace ins {

template <typename T>
class dynamic_circular_buffer {
public:
  using size_type = typename std::deque<T>::size_type;
  using value_type = typename std::deque<T>::value_type;
  using param_value_type = const value_type&;
  using rvalue_type = value_type&&;
  using iterator = typename std::deque<T>::iterator;
  using const_iterator = typename std::deque<T>::const_iterator;
  using reference = typename std::deque<T>::reference;
  using const_reference = typename std::deque<T>::const_reference;
  using pointer = typename std::deque<T>::pointer;
  using difference_type = typename std::deque<T>::difference_type;

public:
  /*!
    \brief Construct empty circular_buffer with capacity 0
  */
  explicit dynamic_circular_buffer() = default;
  /*!
    \brief Construct empty circular_buffer with given capacity
    \param capacity The buffer capacity
  */
  explicit dynamic_circular_buffer(int capacity) : capacity_(capacity) {}
  dynamic_circular_buffer(const dynamic_circular_buffer &oth) = default;
  dynamic_circular_buffer(dynamic_circular_buffer &&oth) = default;
  ~dynamic_circular_buffer() = default;

  /*!
    \brief Push back, if buffer full, replace first one 
    \param value The value
  */
  void push_back(param_value_type &value) {
    push_back_impl<param_value_type>(value);
  }

  /*!
    \brief Push back, if buffer full, replace first one
    \param value The value
  */
  void push_back(rvalue_type value) {
    push_back_impl<rvalue_type>(std::move(value));
  }

  /*!
    \brief Push front, if buffer full, replace last one.
    \param value The value
  */
  void push_front(param_value_type &value) {
    push_front_impl<param_value_type>(value);
  }
  /*!
    \brief Push front, if buffer full, replace last one.
    \param value The value
  */
  void push_front(rvalue_type value) {
    push_front_impl<rvalue_type>(value);
  }

  /*!
    \brief Pop last element, undefined when empty
  */
  void pop_back() {
    data_.pop_back();
  }

  /*!
    \brief Pop first element, undefined when empty
  */
  void pop_front() {
    data_.pop_front();
  }
 
  /*!
    \brief ClearSync all elements
  */
  void clear() {
    data_.clear();
  }

  /*!
    \brief Set new capacity, and will destroy all old elements
    \param new_capacity The new capacity
  */
  void set_capacity(size_type new_capactiy) noexcept {
    if (capacity_ == new_capactiy) return;
    capacity_ = new_capactiy;
    data_.clear();
  }

  /*!
    \brief Get the buffer capacity
    \return new capacity
  */
  size_type capacity() noexcept {
    return capacity_;
  }

  iterator begin() noexcept {
    return data_.begin();
  }

  iterator end() noexcept {
    return data_.end();
  }

  const_iterator begin() const noexcept {
    return data_.begin();
  }

  const_iterator end() const noexcept {
    return data_.end();
  }

  const_iterator cbegin() const noexcept {
    return data_.cbegin();
  }

  const_iterator cend() const noexcept {
    return data_.cend();
  }

  /*!
    \brief Check buffer is empty
    \return If empty, return true
  */
  bool empty() const noexcept {
    return data_.empty();
  }

  /*!
    \brief Check buffer is full
    \return If full, return true
  */
  bool full() const noexcept {
    return size() == capacity_;
  }

  /*!
    \brief Get first element, throw exception when empty
    \return First element
  */
  reference front() {
    return data_.front();
  }

  /*!
    \brief Get first element, throw exception when empty
    \return First element
  */
  const_reference front() const {
    return data_.front();
  }

  /*!
    \brief Get last element, throw exception when empty
    \return Last element
  */
  reference back() {
    return data_.back();
  }

  /*!
    \brief Get last element, throw exception when empty
    \return Last element
  */
  const_reference back() const {
    return data_.back();
  }

  /*!
    \brief Get current circular buffer size.
    \return Buffer size
  */
  size_type size() noexcept {
    return data_.size();
  }

  /*!
    \brief Get max size a circular buffer can alloc
    \return Max buffer size
  */
  size_type max_size() noexcept {
    return data_.max_size();
  }

  /*!
    \brief Access element at pos with bound checking, may thow out_of_range exception
    \param pos Element position
    \return Element reference at pos
  */
  reference at(size_type pos) {
    return data_.at(pos);
  }

  /*!
    \brief Const version for at
  */
  const_reference at(size_type pos) const {
    return data_.at(pos);
  }

  /*!
    \brief Access element at pos without bound checking.
    \param pos Element pos
    \return Element reference
  */
  value_type& operator[](size_type pos) {
    return data_[pos];
  }

  /*!
  \brief Const version for operator[]
  */
  const value_type& operator[](size_type pos) const {
    return data_[pos];
  }

  dynamic_circular_buffer& operator=(const dynamic_circular_buffer &oth) = default;
  dynamic_circular_buffer& operator=(dynamic_circular_buffer &&oth) noexcept = default;

  void swap(dynamic_circular_buffer<T> &cb) noexcept {
    using std::swap;
    swap(capacity_, cb.capacity_);
    swap(data_, cb.data_);
  }

private:
  template <typename DataType>
  void push_back_impl(DataType item) {
    if (capacity_ <= 0) return;
    if (size() >= capacity_) {
      data_.pop_front();
    }
    data_.push_back(item);
  }

  template <typename DataType>
  void push_front_impl(DataType item) {
    if (capacity_ <= 0) return;
    if (size() >= capacity_) {
      data_.pop_back();
    }
    data_.push_front(item);
  }

private:
  size_type capacity_ = 0;
  std::deque<T> data_;
};

//overload swap
template <typename T>
inline void swap(dynamic_circular_buffer<T> &lhs, dynamic_circular_buffer<T> &rhs) {
  lhs.swap(rhs);
}

}



