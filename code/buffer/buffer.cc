#include "buffer.h"

namespace TimelineServer {

// 读字节函数

size_t Buffer::ReadableBytes() const { return write_pos_ - read_pos_; }

const char* Buffer::GetPeek() const { return BeginPtr_() + read_pos_; }

void Buffer::MovePeek(size_t len) { read_pos_ += len; }

std::string Buffer::Read(size_t len) {
  // 取较小值
  len = len > ReadableBytes() ? ReadableBytes() : len;

  std::string str(GetPeek(), GetPeek() + len);
  read_pos_ += len;
  return str;
}

std::string Buffer::ReadAll() { return Read(ReadableBytes()); }

// 写入函数

void Buffer::Write(const std::string& str) { Write(str.data(), str.size()); }

void Buffer::Write(const char* str, size_t len) {
  assert(str);
  MakeSpace_(len);
  std::copy(str, str + len, WritePtr_());
  write_pos_ += len;
}

void Buffer::Write(const void* data, size_t len) {
  assert(data);
  Write(static_cast<const char*>(data), len);
}

void Buffer::Write(const Buffer& buff) {
  Write(buff.GetPeek(), buff.ReadableBytes());
}

// 文件接口

ssize_t Buffer::ReadFd(int fd, int* Errno) {
  char buff[65535];
  struct iovec iov[2];
  const size_t writable = buffer_.size() - write_pos_;

  // 优先直接写入 buffer
  iov[0].iov_base = WritePtr_();
  iov[0].iov_len = writable;
  // 如果文件大小超出 buffer 大小,则写在上面申请的 buff 数组中
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  // 读取文件并写入上述两个地方
  const ssize_t len = readv(fd, iov, 2);

  if (len < 0) {
    *Errno = errno;
    return len;
  } else if (static_cast<size_t>(len) <= writable) {
    // buffer 大小足够,没有写入申请的 buff 中
    write_pos_ += len;
  } else {
    // buffer 不够大,再使用 Write 函数将 buff 中的内容转移到 buffer 中
    write_pos_ = buffer_.size();
    Write(buff, len - writable);
  }

  return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
  size_t read_size = ReadableBytes();
  ssize_t len = write(fd, GetPeek(), read_size);

  if (len < 0) {
    *Errno = errno;
    return len;
  }

  read_pos_ += len;
  return 0;
}

// 私有函数

void Buffer::MakeSpace_(size_t len) {
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
    size_t readable_bytes = ReadableBytes();
    std::copy(ReadPtr_(), WritePtr_(), &(*buffer_.begin()));
    read_pos_ = 0;
    write_pos_ = readable_bytes;
    assert(readable_bytes == ReadableBytes());
  }

  // 只能额外申请空间
  // 直接大方地申请 len 长度
  buffer_.resize(buffer_.size() + len);
}

}  // namespace TimelineServer