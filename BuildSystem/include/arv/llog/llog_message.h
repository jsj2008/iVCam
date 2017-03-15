//
// Created by jerett on 16/4/6.
//

#ifndef LLOG_LLOG_MESSAGE_H
#define LLOG_LLOG_MESSAGE_H

#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <thread>

#if (_WIN32)
#define PATH_SEPARATOR "\\"
#else 
#define PATH_SEPARATOR "/"
#endif 

namespace {
template <typename T> void Ignore(T &&) { }
}

namespace ins {
class LLogMessage {
public:
  LLogMessage(const std::string &file,
              const std::string &function,
              int line,
              int level);

  LLogMessage(const std::string &expression,
              const std::string &file,
              const std::string &function,
              int line);

  LLogMessage(const LLogMessage&) = delete;
  LLogMessage(LLogMessage &&rhs) = delete;
  LLogMessage& operator=(const LLogMessage&) = delete;
  LLogMessage& operator=(LLogMessage&&) = delete;
  ~LLogMessage();

  template <typename T>
  inline LLogMessage& operator<<(const T &msg) {
    oss_ << msg;
    return *this;
  }

  std::ostringstream& oss() noexcept {
    return oss_;
  }

private:
  std::string ParseFileName(const std::string &file) {
    auto pos = file.find_last_of(PATH_SEPARATOR);
    if (pos != std::string::npos) {
      auto filename = file.substr(pos + 1);
      return filename;
    } else {
      return file;
    }
  }

private:
  std::ostringstream oss_;
  bool fatal_ = false;
};
}


#endif //LLOG_LLOG_MESSAGE_H
