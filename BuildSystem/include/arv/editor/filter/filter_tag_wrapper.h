//
// Created by jerett on 16/12/20.
//

#ifndef INSPLAYER_FILTER_TAG_WRAPPER_H
#define INSPLAYER_FILTER_TAG_WRAPPER_H

#include "media_filter.h"

namespace ins {

template <typename TagType, typename ContentType>
struct ContentWithTag {
  TagType tag;
  ContentType content;
};

template <typename TagType, typename FilterType>
class FilterTagWrapper : public MediaFilter<typename FilterType::out_type, ContentWithTag<TagType,  typename FilterType::out_type>> {
public:
  FilterTagWrapper(const TagType &tag) noexcept : tag_(tag) {}
  ~FilterTagWrapper() noexcept = default;

  void SetOpt(const std::string &key, const any &value) override {
    return;
  }

  bool Init(MediaBus &bus) override {
    return this->next_filter_->Init(bus);
  }

  void Close() override {
    return this->next_filter_->Close();
  }

  bool Filter(const typename FilterType::out_type &in_data) override {
    typename FilterTagWrapper::out_type tag_contet;
    tag_contet.tag = tag_;
    tag_contet.content = in_data;
    return this->next_filter_->Filter(tag_contet);
  }

private:
  TagType tag_;
};


}

#endif //INSPLAYER_FILTER_TAG_WRAPPER_H
