#include "log.h"

namespace TimelineServer {

void Log::init(LOG_LEVEL level, const char* path, const char* suffix,
               int max_capacity) {
  level_ = level;
  if (max_capacity) {
    is_async_ = true;
    if (!log_queue_) {
      // make_unique 在 C++14 才引入
      // log_queue_ = std::unique_ptr<LogQueue<string>>(new LogQueue<string>);
      log_queue_ = std::make_unique<LogQueue<string>>();

      write_thread_ = std::make_unique<std::thread>(async_thread_func);
    }
  } else {
    is_async_ = false;
  }

  line_count_ = 0;
  path_ = path;
  suffix_ = suffix;

  // 获取时间
  time_t timestamp = time(nullptr);
  struct tm* sys_time = localtime(&timestamp);
  struct tm t = *sys_time;

  day_of_month_ = t.tm_mday;

  char file_name[LOG_NAME_LEN] = {0};
  snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d-0%s", path_,
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);

  {
    std::lock_guard<std::mutex> locker(fp_mtx_);
    if (fp_) {
      flush();
      fclose(fp_);
    }

    fp_ = fopen(file_name, "a");
    if (fp_ == nullptr) {
      // 权限全开
      mkdir(path_, 0777);
      fp_ = fopen(file_name, "a");
    }

    // 再出错就不礼貌了
    assert(fp_ != nullptr);
  }
}

Log* Log::get_instance() {
  static Log instance;
  return &instance;
}

void Log::write_buffer(LOG_LEVEL level, const char* format, ...) {
  // now 的最小精度为 Microseconds
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm* sysTime = localtime(&tSec);
  struct tm t = *sysTime;

  // 实际参数
  va_list args;

  // 日期变更/达到单文件行数上限,新建日志文件
  if (day_of_month_ != t.tm_mday ||
      (line_count_ && (line_count_ % MAX_LINES == 0))) {
    std::lock_guard<std::mutex> locker(fp_mtx_);

    char new_file[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday);

    if (day_of_month_ != t.tm_mday) {
      // 因为日期更换日志文件
      snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-0%s", path_, tail, suffix_);
      day_of_month_ = t.tm_mday;
      line_count_ = 0;

    } else {
      // 因为行数超限更换文件
      snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
               (line_count_ / MAX_LINES), suffix_);
    }

    flush();
    fclose(fp_);
    fp_ = fopen(new_file, "a");

    assert(fp_ != nullptr);
  }

  {
    std::lock_guard<std::mutex> locker(fp_mtx_);
    line_count_++;

    buffer_.make_space(128);
    int n = snprintf(buffer_.get_write_ptr(), 128,
                     "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900,
                     t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                     now.tv_usec);
    buffer_.move_write_ptr(n);

    log_message_level(level);

    va_start(args, format);
    buffer_.make_space(MAX_CHARS_IN_LINE);
    int m = vsnprintf(buffer_.get_write_ptr(), MAX_CHARS_IN_LINE, format, args);
    buffer_.move_write_ptr(m);
    va_end(args);

    buffer_.make_space(2);
    buffer_.write_buffer("\n\0", 2);

    if (is_async_ && log_queue_ && !log_queue_->is_full()) {
      log_queue_->push(buffer_.read_all());
    } else {
      fputs(buffer_.get_read_ptr(), fp_);
    }
    buffer_.clear();
  }
}

void Log::flush() {
  if (is_async_) {
    log_queue_->flush();
  }
  fflush(fp_);
}

Log::Log() {
  line_count_ = 0;
  is_async_ = false;
  write_thread_ = nullptr;
  log_queue_ = nullptr;
  day_of_month_ = 0;
  fp_ = nullptr;
}

Log::~Log() {
  // 需要释放的资源有两个:打开的文件,开启的另一个写线程(如果有的话)
  if (write_thread_ && write_thread_->joinable()) {
    // 将剩余未写入的内容全部写入
    while (!log_queue_->is_empty()) {
      log_queue_->flush();
    }
    log_queue_->close();
    write_thread_->join();
  }

  {
    std::lock_guard<std::mutex> locker(fp_mtx_);
    if (fp_) {
      flush();
      fclose(fp_);
    }
  }
}

void Log::async_thread_func() { get_instance()->async_write(); }

void Log::async_write() {
  string log_message = "";

  while (log_queue_->pop(log_message)) {
    std::lock_guard<std::mutex> locker(fp_mtx_);
    fputs(log_message.data(), fp_);
  }
}

void Log::log_message_level(LOG_LEVEL level) {
  switch (level) {
    case LOG_LEVEL::ELL_DEBUG:
      buffer_.write_buffer("[DEBUG]", 9);
      break;
    case LOG_LEVEL::ELL_INFO:
      buffer_.write_buffer("[INFO] ", 9);
      break;
    case LOG_LEVEL::ELL_WARN:
      buffer_.write_buffer("[WARN] ", 9);
      break;
    case LOG_LEVEL::ELL_ERROR:
      buffer_.write_buffer("[ERROR]", 9);
      break;
    default:
      buffer_.write_buffer("[DEBUG]", 9);
      break;
  }
}

}  // namespace TimelineServer