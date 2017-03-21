//
// Created by jerett on 16/5/23.
//

#ifndef EVENTLOOP_ANY_H
#define EVENTLOOP_ANY_H

#include <typeinfo>
#include <memory>
namespace ins{

class any;
template<typename T> T any_cast(const any &operand) noexcept (false);

class any {
  template<typename T> friend T any_cast(const any &operand);

public:
  any() = default;

  any(const any &rhs)
      : place_holder_(rhs.place_holder_ ? rhs.place_holder_->clone(): nullptr){
//    printf("any copy any...\n");
  }

  template <typename ValueType>
  any(const ValueType &val)
      : place_holder_(new holder<typename std::decay<ValueType>::type>(val)) {
//    printf("any copy value...\n");
  }

  any& operator=(const any &rhs) noexcept(false) {
    any(rhs).swap(*this);
    return *this;
  }

  any& operator=(any &&rhs) noexcept {
    swap(rhs);
    any().swap(rhs);
    return *this;
  }

  template <typename ValueType> any& operator=(ValueType &&val) {
    any(std::forward<ValueType>(val)).swap(*this);
    return *this;
  }

  any& swap(any &rhs) noexcept {
    place_holder_.swap(rhs.place_holder_);
    return *this;
  }

  template <typename ValueType>
  any(ValueType &&val
      , typename std::enable_if<!std::is_same<any&, ValueType>::value>::type* = 0 )
      : place_holder_(new holder<typename std::decay<ValueType>::type>(std::forward<ValueType>(val))) {
//    printf("any move value...\n");
  }

  ~any() = default;

  bool empty() const noexcept {
    return place_holder_ == nullptr;
  }

  const std::type_info& type() const noexcept {
    return place_holder_ ? place_holder_->type() : typeid(void);
  }

private:
  class placeholder {
  public:
    virtual ~placeholder() {}

    virtual placeholder* clone() const = 0;
    virtual const std::type_info& type() const = 0;
  };

  template <typename ValueType>
  class holder : public placeholder {
  public:
    ~holder() {
    }

    holder(ValueType &&val) : val_(std::forward<ValueType>(val)) {
//      printf("holder move....\n");
    }

    holder(const ValueType& value) : val_(value){
      //printf("holder copy....\n");
    }

    virtual placeholder* clone() const {
      return new holder(val_);
    }

    const std::type_info& type() const noexcept {
      return typeid(ValueType);
    }

  public:
    ValueType val_;
  };

private:
  std::unique_ptr<placeholder> place_holder_;

};

template<typename ValueType> ValueType any_cast(const any &operand) noexcept(false) {
  using DECAY_TYPE = typename std::decay<ValueType>::type;
  if (typeid(DECAY_TYPE) != operand.type()) {
    throw std::bad_cast();
  }
  any::holder<DECAY_TYPE> *holder =
      dynamic_cast<any::holder<DECAY_TYPE>*>(operand.place_holder_.get());
  return holder->val_;
}

}

#endif //EVENTLOOP_ANY_H
