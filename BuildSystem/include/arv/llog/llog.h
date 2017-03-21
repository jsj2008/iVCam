//
// Created by jerett on 16/4/5.
//

#ifndef LLOG_LLOG_H
#define LLOG_LLOG_H

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>

#include "llog_message.h"


#if defined(_WIN32) || defined(_WIN64)
#   define ELPP_OS_WINDOWS 1
#endif

#define LOG(level) \
  if (level >= ins::Configuration::GetInstance().log_level()) \
      ins::LLogMessage(__FILE__, __FUNCTION__, __LINE__, level)

#define LOG_IF(level, expression) \
  if (level >= ins::Configuration::GetInstance().log_level() && true == expression) \
      ins::LLogMessage(__FILE__, __FUNCTION__, __LINE__, level)
//    LOG(level)

#define CHECK(expression) \
  if (false == (expression)) \
      ins::LLogMessage(#expression, __FILE__ , __FUNCTION__, __LINE__)
//    ins::FatalLog(#expression, __FILE__, __FUNCTION__, __LINE__)

#define TimedBlock(obj, tag) \
  ins::PerformanceTrackingObj obj(tag);
  

namespace ins {
class LLogMessage;
extern bool log_to_stderr;
}

#undef VERBOSE
#undef INFO
#undef WARNING
#undef ERROR

enum LogLevel {
  VERBOSE = 0,
  INFO,
  WARNING,
  ERROR,
};

namespace ins{

class Configuration {
public:
  enum {
    LOG_LEVEL = 0,
    LOG_FILE,
  };

  static Configuration& GetInstance() noexcept {
    static Configuration configuration;
    return configuration;
  }

  Configuration(const Configuration&) = delete;
  Configuration(Configuration&&) = delete;

  bool Configure(int type, const std::string &val) noexcept ;

  LogLevel log_level() const noexcept {
    return log_level_;
  }

private:
  Configuration() = default;

private:
  std::string log_file_;
  LogLevel  log_level_ = VERBOSE;
};

class LLog {
public:
  static LLog& Instance() noexcept {
    static LLog llog;
    return llog;
  }

  ~LLog() {
    if (log_file_) {
      fclose(log_file_);
    }
  }

  bool DupLogToFile(const std::string &file) noexcept ;

  LLog(const LLog&) = delete;
  LLog(LLog&&) = delete;

  inline LLog& operator<<(const std::string &msg) {
#if _WIN32
    std::lock_guard<std::mutex> lck(mtx_);
#endif // _WIN3
    fwrite(msg.c_str(), 1, msg.length(), log_to_stderr ? stderr : stdout);
    if (log_file_ != nullptr) {
      fwrite(msg.c_str(), 1, msg.length(), log_file_);
      fflush(log_file_);
    }
    return *this;
  }

private:
  LLog() {}
  FILE *log_file_ = nullptr;
#if _WIN32
  std::mutex mtx_;
#endif
};

/*############Util Function#####################*/
class Timer {
public:
  Timer() noexcept : now_(std::chrono::steady_clock::now()) { ; }
  ~Timer() noexcept = default;

  int64_t Pass() const {
    auto pass = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now_).count();
    return pass;
  }

  void Reset() {
    now_ = std::chrono::steady_clock::now();
  }

private:
  std::chrono::steady_clock::time_point now_;
};

class PerformanceTrackingObj {
public:
  PerformanceTrackingObj(const std::string &block_name) noexcept : block_name_(block_name) {}
  ~PerformanceTrackingObj() noexcept {
    LOG(INFO) << "[" << block_name_ << "]" << " in [" << timer.Pass() << " ms]";
  }

private:
  std::string block_name_;
  Timer timer;
};

}


#endif //LLOG_LLOG_H
