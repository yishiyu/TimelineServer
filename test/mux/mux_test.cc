#include "mux/mux.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {

class MuxTest : public ::testing::Test {
 protected:
  // 初始化数据
  void SetUp() override {
    int ans = pipe(pipe_fds);
    assert(ans == 0);

    base_events = EPOLLONESHOT | EPOLLET | EPOLLHUP;
  }

  // override TearDown 来清理数据
  void TearDown() override {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
  }

  const int timeout = 1000;
  std::vector<struct epoll_event> events;
  int pipe_fds[2];

  Mux mux;
  uint32_t base_events;
};

TEST_F(MuxTest, all) {
  // 创建监听事件(可写)
  mux.add_fd(pipe_fds[1], base_events | EPOLLOUT);

  // 监听可读事件
  int events_count = mux.wait(timeout);

  EXPECT_TRUE(events_count > 0);
  EXPECT_TRUE(mux.get_signal_fd(0) == pipe_fds[1]);
  EXPECT_TRUE(mux.get_signal_events(0) & EPOLLOUT);
}

}  // namespace TimelineServer