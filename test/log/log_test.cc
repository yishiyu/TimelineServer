#include "log/log.h"

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

// TEST(Buffer, get_readable_bytes) {
//   Buffer buffer(5);
//   std::string result;
//   buffer.write_buffer(input);

//   EXPECT_TRUE(buffer.get_readable_bytes() == input.size());

//   int move_size = 10;
//   buffer.move_read_ptr(move_size);
//   EXPECT_TRUE(buffer.get_readable_bytes() == (input.size() - move_size));
// }

}  // namespace TimelineServer