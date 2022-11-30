#include "buffer/buffer.h"

#include <fcntl.h>
#include <gtest/gtest.h>

#include <iostream>
#include <string>

namespace TimelineServer {

TEST(Buffer, ReadAll) {
  Buffer buffer(5);
  std::string input("hello world!!!!!!!");
  buffer.Write(input);

  std::cout << buffer.ReadAll() << std::endl;

  buffer.Write(input);
  buffer.Write(input);
  buffer.Write(input);
  std::cout << buffer.ReadAll() << std::endl;

  //   int fd = open("../LICENSE", O_RDONLY);
  //   int error = 0;

  //   buffer.ReadFd(fd, &error);
  //   std::cout << buffer.ReadAll() << std::endl;
}

}  // namespace TimelineServer