#include "buffer/buffer.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <iostream>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {

const std::string input(
    "Life comes in a package. This package includes happiness and sorrow, "
    "failure and success, hope and despair. Life is a learning process. "
    "Experiences in life teach us new lessons and make us a better person. "
    "With each passing day we learn to handle various situations.");

TEST(Buffer, get_readable_bytes) {
  Buffer buffer(5);
  std::string result;
  buffer.write_buffer(input);

  EXPECT_TRUE(buffer.get_readable_bytes() == input.size());

  int move_size = 10;
  buffer.move_read_ptr(move_size);
  EXPECT_TRUE(buffer.get_readable_bytes() == (input.size() - move_size));
}

TEST(Buffer, read) {
  Buffer buffer(5);
  std::string result;
  buffer.write_buffer(input);

  result = string(buffer.get_read_ptr(), buffer.get_readable_bytes());
  assert(result == input);

  int sub_str_len = 10;
  result = buffer.read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));

  result = buffer.read_all();
  EXPECT_TRUE(result == input.substr(sub_str_len, input.size()));
}

TEST(Buffer, write_buffer) {
  Buffer buffer(5);
  std::string result;

  // void write_buffer(const std::string& str);
  buffer.write_buffer(input);
  result = buffer.read_all();
  EXPECT_TRUE(result == input);

  // void write_buffer(const char* str, size_t len);
  // void write_buffer(const void* data, size_t len);
  int sub_str_len = 10;
  buffer.write_buffer(input.data(), sub_str_len);
  buffer.write_buffer(static_cast<const void*>(input.data()), sub_str_len);
  result = buffer.read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));
  result = buffer.read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));

  // void write_buffer(const Buffer& buff);
  buffer.write_buffer(input.substr(0, sub_str_len));
  buffer.write_buffer(buffer);
  result = buffer.read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));
  result = buffer.read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));

  string text = "123456";
  buffer.write_buffer("123456", 6);
  result = buffer.read(6);
  EXPECT_TRUE(text == result);
}

TEST(Buffer, File) {
  Buffer buffer(5);
  // Building CXX object CMakeFiles/buffer_test.dir/test/buffer/buffer_test.cc.o
  const string file_name("../data/test/buffer/buffer_read.txt");
  const string dest_file("../data/test/buffer/buffer_write.txt");

  // 读取文件
  FILE* fd = fopen(file_name.data(), "r");
  if (nullptr == fd) {
    cout << "Cannot open " << file_name << ". errno:" << errno << endl;
    EXPECT_TRUE(false);
  }
  int error;
  int len = buffer.read_fd(fd->_fileno, &error);
  fclose(fd);

  if (len < 0) {
    cout << "Error with " << error << "." << endl;
    EXPECT_TRUE(false);
  }

  // 写入另一个文件
  fd = fopen(dest_file.data(), "w");
  if (nullptr == fd) {
    cout << "Cannot open " << file_name << ". errno:" << errno << endl;
    EXPECT_TRUE(false);
  }
  len = buffer.write_fd(fd->_fileno, &error);
  fclose(fd);

  if (len < 0) {
    cout << "Error with " << error << "." << endl;
    EXPECT_TRUE(false);
  }

  // TODO 验证两个文件相同
  // 手动看了下是一样的(偷个懒捏)
}

}  // namespace TimelineServer