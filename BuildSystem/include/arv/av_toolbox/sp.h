//
// Created by jerett on 16/12/9.
//

#ifndef INSPLAYER_SP_H
#define INSPLAYER_SP_H

#include <memory>

namespace ins {

template <typename T>
using sp = std::shared_ptr<T>;

template <typename T>
using up = std::unique_ptr<T>;
}


#endif //INSPLAYER_SP_H
