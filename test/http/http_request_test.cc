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

    request_file_get = "../data/request_get.txt";
    request_file_post = "../data/request_post.txt";

    fd_get = fopen(request_file_get.data(), "r");
    fd_post = fopen(request_file_post.data(), "r");
  }

  // override TearDown 来清理数据
  void TearDown() override {
    Log::get_instance()->flush();

    fclose(fd_get);
    fclose(fd_post);
  }

  string request_file_get;
  string request_file_post;
  FILE* fd_get;
  FILE* fd_post;
};

TEST_F(HttpRequestTest, get) {
  HttpRequest request;
  Buffer buffer;
  int error_flag = 0;

  int len = buffer.read_fd(fd_get->_fileno, &error_flag);

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

TEST_F(HttpRequestTest, post) {
  HttpRequest request;
  Buffer buffer;
  int error_flag = 0;

  int len = buffer.read_fd(fd_post->_fileno, &error_flag);

  cout << "request length:" << len << endl;

  request.parse(buffer);

  cout << "==========request head infomation==========" << endl;
  auto post = request.get_post();

  for (auto pair : post.object_items()) {
    EXPECT_TRUE(pair.second == request.query_post(pair.first));
    // 非string的json值string_value会直接返回"",不是错误
    cout << pair.first << ":" << pair.second.string_value() << endl;
  }
}

}  // namespace TimelineServer