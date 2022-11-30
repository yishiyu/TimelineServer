#pragma once

#include <assert.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <vector>

namespace TimelineServer {

class Buffer {
 public:
  Buffer(int size = 1024) : buffer_(size), read_pos_(0), write_pos_(0){};
  ~Buffer() = default;

  // 读取缓存(指针式操作)(获取可读字节数量/获取读取指针/移动写指针)
  size_t ReadableBytes() const;
  const char* GetPeek() const;
  void MovePeek(size_t len);

  // 读取缓存(string式操作)
  std::string Read(size_t len);
  std::string ReadAll();

  // 写入缓存
  void Write(const std::string& str);
  void Write(const char* str, size_t len);
  void Write(const void* data, size_t len);
  void Write(const Buffer& buff);

  // 文件接口
  ssize_t ReadFd(int fd, int* Errno);
  ssize_t WriteFd(int fd, int* Errno);

 private:
  const char* BeginPtr_() const { return &(*buffer_.begin()); };
  char* ReadPtr_() { return &(*buffer_.begin()) + read_pos_; }
  char* WritePtr_() { return &(*buffer_.begin()) + write_pos_; }

  // 保证有这么多空间可以写入
  void MakeSpace_(size_t len);

  std::vector<char> buffer_;
  std::atomic<std::size_t> read_pos_;
  std::atomic<std::size_t> write_pos_;
};

}  // namespace TimelineServer