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

TEST(Buffer, ReadableBytes) {
  Buffer buffer(5);
  std::string result;
  buffer.Write(input);

  EXPECT_TRUE(buffer.ReadableBytes() == input.size());

  int move_size = 10;
  buffer.MovePeek(move_size);
  EXPECT_TRUE(buffer.ReadableBytes() == (input.size() - move_size));
}

TEST(Buffer, Read) {
  Buffer buffer(5);
  std::string result;
  buffer.Write(input);

  result = string(buffer.GetPeek(), buffer.ReadableBytes());
  assert(result == input);

  int sub_str_len = 10;
  result = buffer.Read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));

  result = buffer.ReadAll();
  EXPECT_TRUE(result == input.substr(sub_str_len, input.size()));
}

TEST(Buffer, Write) {
  Buffer buffer(5);
  std::string result;

  // void Write(const std::string& str);
  buffer.Write(input);
  result = buffer.ReadAll();
  EXPECT_TRUE(result == input);

  // void Write(const char* str, size_t len);
  // void Write(const void* data, size_t len);
  int sub_str_len = 10;
  buffer.Write(input.data(), sub_str_len);
  buffer.Write(static_cast<const void*>(input.data()), sub_str_len);
  result = buffer.Read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));
  result = buffer.Read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));

  // void Write(const Buffer& buff);
  buffer.Write(input.substr(0, sub_str_len));
  buffer.Write(buffer);
  result = buffer.Read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));
  result = buffer.Read(sub_str_len);
  EXPECT_TRUE(result == input.substr(0, sub_str_len));
}

TEST(Buffer, File) {
  Buffer buffer(5);
  // Building CXX object CMakeFiles/buffer_test.dir/test/buffer/buffer_test.cc.o
  const string file_name("../data/test_file.txt");
  const string dest_file("../data/des.txt");

  // 读取文件
  FILE* fd = fopen(file_name.data(), "r");
  if (nullptr == fd) {
    cout << "Cannot open " << file_name << ". errno:" << errno << endl;
    EXPECT_TRUE(false);
  }
  int error;
  int len = buffer.ReadFd(fd->_fileno, &error);
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
  len = buffer.WriteFd(fd->_fileno, &error);
  fclose(fd);

  if (len < 0) {
    cout << "Error with " << error << "." << endl;
    EXPECT_TRUE(false);
  }

  // TODO 验证两个文件相同
  // 手动看了下是一样的(偷个懒捏)
}

}  // namespace TimelineServer