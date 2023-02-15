#pragma once

#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "log/log.h"

#include <vector>

namespace TimelineServer {

class Mux {
 public:
  explicit Mux(int max_event = 512);
  ~Mux();

  // 数据源操作函数
  bool add_fd(int fd, uint32_t events);
  bool mod_fd(int fd, uint32_t events);
  bool del_fd(int fd);

  // 查询函数/获取信号来源
  int wait(int timeout = -1);
  int get_active_fd(int i) const;
  int get_active_events(int i) const;

 private:
  int mux_fd_;

  // 当前触发事件的信号源数量
  int events_count_;
  std::vector<struct epoll_event> events_;
};

}  // namespace TimelineServer
