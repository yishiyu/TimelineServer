#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <vector>

namespace TimelineServer {

class Buffer {
 public:
  Buffer(int size = 1024) : buffer_(size), read_pos_(0), write_pos_(0){};
  ~Buffer() = default;

  // 读取缓存(指针式操作)(获取可读字节数量/获取读取指针/移动读取指针)
  size_t get_readable_bytes() const;
  char* get_read_ptr();
  void move_read_ptr(size_t len);

  // 写入缓存(指针式操作)
  // 保证有这么多空间可以写入
  void make_space(size_t len);
  char* get_write_ptr();
  void move_write_ptr(size_t len);

  // 读取缓存(string式操作)
  std::string read(size_t len);
  std::string read_all();

  void clear();

  // 写入缓存
  void write_buffer(const std::string& str);
  void write_buffer(const char* str, size_t len);
  void write_buffer(const void* data, size_t len);
  void write_buffer(Buffer& buff);

  // 文件接口
  ssize_t read_fd(int fd, int* Errno);
  ssize_t write_fd(int fd, int* Errno);

 private:
  std::vector<char> buffer_;
  std::atomic<std::size_t> read_pos_;
  std::atomic<std::size_t> write_pos_;
};

}  // namespace TimelineServer