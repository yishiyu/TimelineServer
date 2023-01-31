#include "buffer.h"

namespace TimelineServer {

// 读字节函数

size_t Buffer::get_readable_bytes() const { return write_pos_ - read_pos_; }

char* Buffer::get_read_ptr() { return &(*buffer_.begin()) + read_pos_; }
char* Buffer::get_write_ptr() { return &(*buffer_.begin()) + write_pos_; }

void Buffer::move_read_ptr(size_t len) {
  if (len > (write_pos_ - read_pos_)) {
    read_pos_ = write_pos_ = 0;
  } else {
    read_pos_ += len;
  }
}

void Buffer::make_space(size_t len) {
  // 本身就有足够的空间
  // 1024(size) - 1023(write_pos_) = 1(剩余1023这个位置)
  int remaining_space = buffer_.size() - write_pos_;
  if (remaining_space > len) {
    return;
  }

  // 加上读取过的空白空间足够
  // 1(read_pos_) = 1(剩余1)
  int used_space = read_pos_;
  if (remaining_space + used_space > len) {
    // 将已写入内容前移
    size_t readable_bytes = get_readable_bytes();
    std::copy(get_read_ptr(), get_write_ptr(), &(*buffer_.begin()));
    read_pos_ = 0;
    write_pos_ = readable_bytes;
    assert(readable_bytes == get_readable_bytes());
  }

  // 只能额外申请空间
  // 直接大方地申请 len 长度
  buffer_.resize(buffer_.size() + len);
}

void Buffer::move_write_ptr(size_t len) {
  // 使用这个函数很危险,但在使用 snprintf 函数的时候必须这么用
  // 在使用这个函数之前必须使用 make_space 扩展出足够的空间
  assert(buffer_.size() >= len);
  write_pos_ += len;
}

std::string Buffer::read(size_t len) {
  // 取较小值
  len = len > get_readable_bytes() ? get_readable_bytes() : len;

  std::string str(get_read_ptr(), get_read_ptr() + len);
  read_pos_ += len;
  return str;
}

std::string Buffer::read_all() { return read(get_readable_bytes()); }

void Buffer::clear() {
  read_pos_ = 0;
  write_pos_ = 0;
}

// 写入函数

void Buffer::write_buffer(const std::string& str) {
  write_buffer(str.data(), str.size());
}

void Buffer::write_buffer(const char* str, size_t len) {
  assert(str);
  make_space(len);
  std::copy(str, str + len, get_write_ptr());
  write_pos_ += len;
}

void Buffer::write_buffer(const void* data, size_t len) {
  assert(data);
  write_buffer(static_cast<const char*>(data), len);
}

void Buffer::write_buffer(Buffer& buff) {
  write_buffer(buff.get_read_ptr(), buff.get_readable_bytes());
}

// 文件接口

ssize_t Buffer::read_fd(int fd, int* errno_) {
  char buff[65535];
  struct iovec iov[2];
  const size_t writable = buffer_.size() - write_pos_;

  // 优先直接写入 buffer
  iov[0].iov_base = get_write_ptr();
  iov[0].iov_len = writable;
  // 如果文件大小超出 buffer 大小,则写在上面申请的 buff 数组中
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  // 读取文件并写入上述两个地方
  const ssize_t len = readv(fd, iov, 2);

  if (len < 0) {
    *errno_ = errno;
    return len;
  } else if (static_cast<size_t>(len) <= writable) {
    // buffer 大小足够,没有写入申请的 buff 中
    write_pos_ += len;
  } else {
    // buffer 不够大,再使用 write_buffer 函数将 buff 中的内容转移到 buffer 中
    write_pos_ = buffer_.size();
    write_buffer(buff, len - writable);
  }

  return len;
}

ssize_t Buffer::write_fd(int fd, int* errno_) {
  size_t read_size = get_readable_bytes();
  ssize_t len = write(fd, get_read_ptr(), read_size);

  if (len < 0) {
    *errno_ = errno;
    return len;
  }

  read_pos_ += len;
  return 0;
}

}  // namespace TimelineServer