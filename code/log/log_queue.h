#pragma once

#include <assert.h>
#include <sys/time.h>

#include <condition_variable>
#include <deque>
#include <mutex>

template <class T>
class LogQueue {
 public:
  explicit LogQueue(size_t max_capacity = 1000);

  ~LogQueue();

  bool is_empty();
  bool is_full();

  void close();

  size_t size();
  size_t capacity();

  bool push(const T& item);
  bool pop(T& item);

  // 针对log的功能
  void flush();

 private:
  // 保证双向队列安全
  std::mutex deq_mtx_;
  std::deque<T> deq_;

  // 限定最大长度(Log过多时会丢弃一部分Log)
  size_t capacity_;

  bool is_closed_;

  std::condition_variable cond_consumer_;
  std::condition_variable cond_producer_;
};

template <class T>
LogQueue<T>::LogQueue(size_t max_capacity) : capacity_(max_capacity) {
  assert(max_capacity > 0);
  is_closed_ = false;
}

template <class T>
LogQueue<T>::~LogQueue() {
  close();
}

template <class T>
bool LogQueue<T>::is_empty() {
  std::lock_guard<std::mutex> locker(deq_mtx_);
  return deq_.empty();
}

template <class T>
bool LogQueue<T>::is_full() {
  std::lock_guard<std::mutex> locker(deq_mtx_);
  return deq_.size >= capacity_;
}

template <class T>
void LogQueue<T>::close() {
  {
    std::lock_guard<std::mutex> locker(deq_mtx_);
    // 直接舍弃所有Log
    deq_.clear();
    is_closed_ = true;
  }

  // 其他所有用该队列的线程都会检测 is_closed 并退出
  cond_consumer_.notify_all();
  cond_producer_.notify_all();
}

template <class T>
size_t LogQueue<T>::size() {
  std::lock_guard<std::mutex> locker(deq_mtx_);
  return deq_.size();
}

template <class T>
size_t LogQueue<T>::capacity() {
  return capacity_;
}

template <class T>
bool LogQueue<T>::push(const T& item) {
  {
    std::unique_lock<std::mutex> locker(deq_mtx_);
    while (deq_.size() >= capacity_) {
      cond_producer_.wait(locker);
      if (is_closed_) {
        return false;
      }
    }
    deq_.push_back(item);
  }

  cond_consumer_.notify_one();
  return true;
}

template <class T>
bool LogQueue<T>::pop(T& item) {
  {
    std::unique_lock<std::mutex> locker(deq_mtx_);
    while (deq_.empty()) {
      cond_consumer_.wait(locker);
      if (is_closed_) {
        return false;
      }
    }
    item = deq_.front();
    deq_.pop_front();
  }

  cond_producer_.notify_one();
  return true;
}

template <class T>
void LogQueue<T>::flush() {
  // 目标场景: 队列不为空,但有多个读者阻塞
  // 虽然没想出来怎么才能到这个场景...
  cond_consumer_.notify_all();
}