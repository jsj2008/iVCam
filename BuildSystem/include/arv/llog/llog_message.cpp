//
// Created by jerett on 16/4/6.
//

#include "llog_message.h"
#include "llog.h"

namespace ins{

LLogMessage::LLogMessage(const std::string &file,
                         const std::string &function,
                         int line,
                         int level) {
  Ignore(function);
  static const char *level_description[] = {
    "VERBOSE",
    "INFO",
    "WARNING",
    "ERROR",
  };
  auto now = std::chrono::system_clock::now();
  auto log_time_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto now_c = std::chrono::system_clock::to_time_t(log_time_ms);
  auto local_time = std::localtime(&now_c);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%m-%d %T", local_time);
//  auto id = std::hash<std::thread::id>()(std::this_thread::get_id());
  auto id = std::this_thread::get_id();

  oss_ << '[' << "0x" << std::hex << id << std::dec << ']';
  oss_ << '[' << level_description[level];
  oss_ << ' ' << time_str << '.' << log_time_ms.time_since_epoch().count() % 1000;
  oss_ << ' ' << ParseFileName(file);
  oss_ << ':' << line << ']';
}

LLogMessage::LLogMessage(const std::string &expression,
                         const std::string &file,
                         const std::string &function,
                         int line) :fatal_(true) {
  Ignore(function);
  auto now = std::chrono::system_clock::now();
  auto log_time_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto now_c = std::chrono::system_clock::to_time_t(log_time_ms);
  auto local_time = std::localtime(&now_c);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%m-%d %T", local_time);
  auto id = std::this_thread::get_id();
  oss_ << '[' << "0x" << std::hex << id << std::dec << ']';
  oss_ << '[' << "Check failed (" << expression << ')';
  oss_ << ' ' << time_str << '.' << log_time_ms.time_since_epoch().count() % 1000;
  oss_ << ' ' << ParseFileName(file);
  oss_ << ':' << line << ']';
}

LLogMessage::~LLogMessage() {
  oss_ << '\n';
  LLog::Instance() << oss_.str();
  if (fatal_) {
#if _WIN32
    system("pause");
#endif // _WIN32
    exit(EXIT_FAILURE);
  }
}

}
