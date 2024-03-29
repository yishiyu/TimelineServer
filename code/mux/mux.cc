#include "mux.h"

const static char LOG_TAG[] = "MUX";

namespace TimelineServer {

Mux::Mux(int max_event) : mux_fd_(epoll_create(max_event)), events_(max_event) {
  assert(mux_fd_ >= 0 && events_.size() > 0);
}

Mux::~Mux() { close(mux_fd_); }

bool Mux::add_fd(int fd, uint32_t events) {
  if (fd < 0) {
    return false;
  }

  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;

  return 0 == epoll_ctl(mux_fd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Mux::mod_fd(int fd, uint32_t events) {
  if (fd < 0) {
    return false;
  }

  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;

  return 0 == epoll_ctl(mux_fd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Mux::del_fd(int fd) {
  if (fd < 0) {
    LOG_WARN("[%s] Delete a negative fd[%d]!", LOG_TAG, fd);
    return false;
  }

  epoll_event ev = {0};

  return 0 == epoll_ctl(mux_fd_, EPOLL_CTL_DEL, fd, &ev);
}

int Mux::wait(int timeout) {
  // While one thread is blocked in a call to epoll_pwait(), 
  // it is possible for another thread to add a file descriptor 
  // to the waited-upon epoll instance. If the new file descriptor 
  // becomes ready, it will cause the epoll_wait() call to unblock. 
  events_count_ = epoll_wait(mux_fd_, &events_[0],
                             static_cast<int>(events_.size()), timeout);
  if (events_count_<0){
    LOG_ERROR("[%s] epoll_wait error with errno:[%d]", LOG_TAG, errno);
  }
  return events_count_;
}

int Mux::get_active_fd(int i) const {
  // epoll_wait 本身是有可能返回 -1 的,不能 assert(i < events_count_)
  assert(i < events_.size() && i >= 0);
  return events_[i].data.fd;
}

int Mux::get_active_events(int i) const {
  // epoll_wait 本身是有可能返回 -1 的,不能 assert(i < events_count_)
  assert(i < events_.size() && i >= 0);
  return events_[i].events;
}

}  // namespace TimelineServer
