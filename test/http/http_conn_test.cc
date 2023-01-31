#include "http/http_conn.h"

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
class HttpConnTest : public ::testing::Test {
 protected:
  // 初始化数据
  void SetUp() override {
    Log::get_instance()->init(LOG_LEVEL::ELL_DEBUG, "../data/test/log", ".log",
                              0);

    request_file_success_path = "../data/test/http/conn_request_success.txt";
    request_file_fail_path = "../data/test/http/conn_request_fail.txt";
    src_dir = "../data/test/http";

    request_file_success = fopen(request_file_success_path.data(), "r+");
    request_file_fail = fopen(request_file_fail_path.data(), "r+");

    connection.global_config(true, src_dir, 0);
  }

  // override TearDown 来清理数据
  void TearDown() override {
    Log::get_instance()->flush();

    fclose(request_file_success);
    fclose(request_file_fail);
  }

  string request_file_success_path;
  string request_file_fail_path;
  string src_dir;
  FILE* request_file_success;
  FILE* request_file_fail;

  HttpConn connection;
};

TEST_F(HttpConnTest, success) {
  // 用文件模拟socket,最后会把回复报文和请求报文写在一块儿
  // 每次测试都需要手动删掉回复报文
  // (不删也行,就这么堆着也行)

  struct sockaddr_in addr = {0};
  connection.init(request_file_success->_fileno, addr);

  int errno_;
  int len = 0;
  len = connection.read(&errno_);
  EXPECT_FALSE(len < 0);

  connection.process();

  len = connection.write(&errno_);
  EXPECT_FALSE(len < 0);
}

TEST_F(HttpConnTest, failed) {

  struct sockaddr_in addr = {0};
  connection.init(request_file_fail->_fileno, addr);

  int errno_;
  int len = 0;
  len = connection.read(&errno_);
  EXPECT_FALSE(len < 0);

  connection.process();

  len = connection.write(&errno_);
  EXPECT_FALSE(len < 0);
}
}  // namespace TimelineServer