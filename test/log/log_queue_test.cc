#include "log/log_queue.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {

// 继承自被测试类,其中的数据可以被多个测试函数使用
class QueueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 初始化数据
    string text =
        "Life comes in a package. This package includes happiness and sorrow, "
        "failure and success, hope and despair. Life is a learning process. "
        "Experiences in life teach us new lessons and make us a better person. "
        "With each passing day we learn to handle various situations.";

    string delimiter = ".";
    size_t pos = 0;
    string token;
    while ((pos = text.find(delimiter)) != string::npos) {
      token = text.substr(0, pos + delimiter.length());
      inputs.push_back(token);
      test_queue.push(token);
      text.erase(0, pos + delimiter.length());
    }
  }

  // override TearDown 来清理数据
  // void TearDown() override {}

  std::vector<string> inputs;
  LogQueue<string> test_queue;
};

TEST_F(QueueTest, pop) {
  string log;

  for (string s : inputs) {
    EXPECT_FALSE(test_queue.is_empty());
    cout << "test_queue.size() = " << test_queue.size() << endl;

    test_queue.pop(log);

    cout << log << endl;
    EXPECT_TRUE(log == s);
  }

  EXPECT_TRUE(test_queue.is_empty());
}

}  // namespace TimelineServer