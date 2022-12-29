#pragma once

#include <assert.h>

#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

typedef std::function<void()> timeout_cb;
typedef unsigned int timer_id;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::microseconds MS;
typedef Clock::time_point time_stamp;

namespace TimelineServer {

struct TimerNode {
  timer_id id;
  time_stamp expires;
  timeout_cb cb;
  bool operator<(const TimerNode& t) { return expires < t.expires; }
};

class Timer {
 public:
  Timer() { heap_.reserve(64); };
  ~Timer() { clear(); };

  // 增加/删除定时器
  void add_timer(timer_id id, int timeout, timeout_cb& call_back);
  void pop_timer();

  // 调用某个定时器的回调函数/重新调整某个定时器的时间
  void do_work(timer_id id);
  void adjust(timer_id id, int timeout);

  // 删除所有定时器/查询定时器是否到时间,激活并删除
  void clear();
  void tick();

  // 获取距离下一个定时器触发的时间差
  int get_next_timeout();

 private:
  void delete_(size_t i);
  void siftup_(size_t i);
  bool siftdown_(size_t i, size_t n);

  void swap_(size_t i, size_t j);
  
  // 小顶堆,0下标对应最小元素
  std::vector<TimerNode> heap_;
  std::unordered_map<timer_id, size_t> timer_ref_;
};

}  // namespace TimelineServer