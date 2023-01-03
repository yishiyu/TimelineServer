#include "timer/timer.h"

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
class TimerTest : public ::testing::Test {
 protected:
  // // 初始化数据
  // void SetUp() override {}

  // override TearDown 来清理数据
  void TearDown() override { timer.clear(); }

  Timer timer;
};

void timeout_message(string msg) { cout << msg << endl; }

TEST_F(TimerTest, add_timer) {
  std::vector<int> timer_delay = {3000, 4000, 5000};

  char message[1024] = {0};

  for (int delay : timer_delay) {
    sprintf(message, "timer %d is called", delay);
    timer.add_timer(delay, delay, std::bind(timeout_message, string(message)));
  }
  while (timer.get_next_timeout() > 0) {
    sleep(1);
  }

  timer.clear();
}

TEST_F(TimerTest, pop_timer) {
  std::vector<int> timer_delay = {11238, 1537, 983, 123, 1380,
                                  4382,  9382, 23,  4659};

  char message[1024] = {0};

  for (int delay : timer_delay) {
    sprintf(message, "timer %d is called", delay);
    timer.add_timer(delay, delay, std::bind(timeout_message, string(message)));
  }

  // 删除最前面两个定时器
  timer.pop_timer();
  timer.pop_timer();

  while (timer.get_next_timeout() > 0) {
    sleep(1);
  }

  timer.clear();
}

TEST_F(TimerTest, do_work) {
  std::vector<int> timer_delay = {3000, 4000, 5000};

  char message[1024] = {0};

  for (int delay : timer_delay) {
    sprintf(message, "timer %d is called", delay);
    timer.add_timer(delay, delay, std::bind(timeout_message, string(message)));
  }

  // 直接执行4000id的任务
  cout << "========do work========" << endl;
  timer.do_work(4000);
  cout << "=======================" << endl;

  while (timer.get_next_timeout() > 0) {
    sleep(1);
  }

  timer.clear();
}

TEST_F(TimerTest, adjust) {
  std::vector<int> timer_delay = {3000, 4000, 5000};

  char message[1024] = {0};

  for (int delay : timer_delay) {
    sprintf(message, "timer %d is called", delay);
    timer.add_timer(delay, delay, std::bind(timeout_message, string(message)));
  }

  // 修改4000id的任务的时间
  cout << "========do work========" << endl;
  cout << "timer 4000 is adjusted" << endl;
  sprintf(message, "timer %d is called", 4000);
  timer.add_timer(4000, 6000, std::bind(timeout_message, string(message)));
  cout << "=======================" << endl;

  while (timer.get_next_timeout() > 0) {
    sleep(1);
  }

  timer.clear();
}

}  // namespace TimelineServer