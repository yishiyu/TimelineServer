#pragma once

#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <mutex>
#include <string>
#include <thread>

#include "buffer/buffer.h"
#include "log/log_queue.h"

using std::string;

namespace TimelineServer {

const int LOG_PATH_LEN = 256;
const int LOG_NAME_LEN = 256;
const int MAX_CHARS_IN_LINE = 1000;
const int MAX_LINES = 50000;

enum LOG_LEVEL {
  ELL_INFO,
  ELL_DEBUG,
  ELL_WARN,
  ELL_ERROR,
};

class Log {
 public:
  void init(LOG_LEVEL level, const char* path = "./log",
            const char* suffix = ".log", int max_capacity = 1000);

  static Log* get_instance();

  void write_buffer(LOG_LEVEL level, const char* format, ...);
  void flush();

  int get_level() { return level_; };
  void set_level(LOG_LEVEL level) { level_ = level; };

 private:
  Log();
  virtual ~Log();

  static void async_thread_func();
  void async_write();
  void log_message_level(LOG_LEVEL level);

  const char* path_;
  const char* suffix_;

  int line_count_;
  int day_of_month_;

  Buffer buffer_;
  LOG_LEVEL level_;
  bool is_async_;

  std::mutex fp_mtx_;
  FILE* fp_;
  std::unique_ptr<LogQueue<string>> log_queue_;
  std::unique_ptr<std::thread> write_thread_;
};

// #define LOG_BASE(level, format, ...) \
//   do {                               \
//     Log* log = Log::get_instance();  \
//     if (log->get_level() <= level) {
//       log->write_buffer(level, format)
// }

// }
// while (0)
}  // namespace TimelineServer