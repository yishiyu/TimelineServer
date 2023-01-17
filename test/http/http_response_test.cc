#include "http/http_response.h"

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
class HttpResponseTest : public ::testing::Test {
 protected:
  // // 初始化数据
  void SetUp() override {
    Log::get_instance()->init(LOG_LEVEL::ELL_DEBUG, "../data/log", ".log", 0);
  }

  // override TearDown 来清理数据
  void TearDown() override { Log::get_instance()->flush(); }
};

TEST_F(HttpResponseTest, success) {
  HttpResponse response;
  response.init("../data/", "test_file.txt", false, 200);

  Buffer buff;

  response.make_response(buff);

  cout << buff.read_all() << endl;
  cout << response.file() << endl;
}

TEST_F(HttpResponseTest, failure) {
  HttpResponse response;
  response.init("../data/", "error_file.txt", false, 200);

  Buffer buff;

  response.make_response(buff);

  cout << buff.read_all() << endl;
  cout << response.file() << endl;
}

}  // namespace TimelineServer