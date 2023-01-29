#include "http/http_request.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include "buffer/buffer.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {
class HttpRequestTest : public ::testing::Test {
 protected:
  // 初始化数据
  void SetUp() override {
    Log::get_instance()->init(LOG_LEVEL::ELL_DEBUG, "../data/log", ".log", 0);

    request_file = "../data/request.txt";
    fd = fopen(request_file.data(), "r");
  }

  // override TearDown 来清理数据
  void TearDown() override {
    Log::get_instance()->flush();

    fclose(fd);
  }

  string request_file;
  FILE* fd;
};

TEST_F(HttpRequestTest, get) {
  HttpRequest request;
  Buffer buffer;
  int error_flag = 0;

  int len = buffer.read_fd(fd->_fileno, &error_flag);

  cout << "request length:" << len << endl;

  request.parse(buffer);

  cout << "==========request line infomation==========" << endl;
  cout << "request path:" << request.get_path() << endl;
  cout << "request method:" << request.get_method() << endl;
  cout << "request version:" << request.get_version() << endl;
  cout << "request path:" << request.get_path() << endl;

  cout << "==========request head infomation==========" << endl;
  auto header = request.get_header();
  for (auto pair : header) {
    EXPECT_TRUE(pair.second == request.query_header(pair.first));
    cout << pair.first << ":" << pair.second << endl;
  }

  cout << "==========get is keep alive==========" << endl;
  string result = request.get_is_keep_alive() ? "true" : "false";
  cout << "is keep alive:" << result << endl;
}

}  // namespace TimelineServer