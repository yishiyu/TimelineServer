#include "buffer.h"

namespace TimelineServer {

// 读字节函数

size_t Buffer::ReadableBytes() const { return write_pos_ - read_pos_; }

const char* Buffer::GetPeek() const { return BeginPtr_() + read_pos_; }

void Buffer::MovePeek(size_t len) { write_pos_ += len; }

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
  // TODO
  return 0;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
  // TODO
  return 0;
}

// 私有函数

void Buffer::MakeSpace_(size_t len) {
  // 本身就有足够的空间
  // 1024(size) - 1023(write_pos_) - 1 = 0(剩余0)
  int remaining_space = buffer_.size() - write_pos_ - 1;
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