#pragma once

#include <assert.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace TimelineServer {

// 不同 Worker 共享的数据
struct Manager {
  std::mutex mtx;
  std::condition_variable cond;
  bool is_closed;
  std::queue<std::function<void()>> tasks;
};

class ThreadPool {
 public:
  explicit ThreadPool(size_t workers = 8)
      : manager_(std::make_shared<Manager>()) {
    assert(workers > 0);
    for (size_t i = 0; i < workers; i++) {
      std::thread([manager = manager_] {
        std::unique_lock<std::mutex> locker(manager->mtx);
        while (true) {
          if (manager->is_closed) break;

          if (!manager->tasks.empty()) {
            auto task = std::move(manager->tasks.front());
            manager->tasks.pop();
            locker.unlock();
            task();
            locker.lock();
          } else {
            manager->cond.wait(locker);
          }
        }
      }).detach();
    }
  }

  ThreadPool() = default;

  // 移动构造
  ThreadPool(ThreadPool&&) = default;

  ~ThreadPool() {
    // https://en.cppreference.com/w/cpp/memory/shared_ptr/operator_bool
    // true if *this stores a pointer, false otherwise.
    if (static_cast<bool>(manager_)) {
      {
        std::lock_guard<std::mutex> locker(manager_->mtx);
        manager_->is_closed = false;
      }
      manager_->cond.notify_all();
    }
  }

  template <class F>
  void add_task(F&& task) {
    {
      std::lock_guard<std::mutex> locker(manager_->mtx);
      manager_->tasks.emplace(std::forward<F>(task));
    }
    manager_->cond.notify_one();
  }

 private:
  std::shared_ptr<Manager> manager_;
};

}  // namespace TimelineServer
