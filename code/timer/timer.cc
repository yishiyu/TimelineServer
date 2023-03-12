#include "timer.h"

const static char LOG_TAG[] = "TIMER";

namespace TimelineServer {

void Timer::add_timer(timer_id id, int timeout, const timeout_cb& call_back) {
  
  assert(id >= 0);
  size_t i;

  if (timer_ref_.count(id) == 0) {
    // 新节点
    i = heap_.size();
    timer_ref_[id] = i;
    heap_.push_back({id, Clock::now() + MS(timeout), call_back});
    siftup_(i);
  } else {
    // 已有节点
    i = timer_ref_[id];
    heap_[i].expires = Clock::now() + MS(timeout);
    heap_[i].cb = call_back;

    // 调整该元素位置
    if (!siftdown_(i, heap_.size())) {
      siftup_(i);
    }
  }
}

void Timer::pop_timer() {
  

  assert(!heap_.empty());
  delete_(0);
}

void Timer::do_work(timer_id id) {
  

  if (heap_.empty() || timer_ref_.count(id) == 0) {
    return;
  }

  size_t i = timer_ref_[id];
  TimerNode& node = heap_[i];
  node.cb();
  delete_(i);
}

void Timer::adjust(timer_id id, int timeout) {
  

  // 保证该元素存在
  assert(!heap_.empty() && timer_ref_.count(id) > 0);
  // 调整触发时间
  heap_[timer_ref_[id]].expires = Clock::now() + MS(timeout);

  // 调整该元素位置
  if (!siftdown_(timer_ref_[id], heap_.size())) {
    siftup_(timer_ref_[id]);
  }
}

void Timer::clear() {
  

  // LOG_DEBUG("[%s] timers are cleared up.", LOG_TAG);
  timer_ref_.clear();
  heap_.clear();
}

void Timer::tick() {
  
  
  if (heap_.empty()) {
    return;
  }

  while (!heap_.empty()) {
    TimerNode& node = heap_.front();
    MS time_left = std::chrono::duration_cast<MS>(node.expires - Clock::now());
    if (time_left.count() > 0) {
      break;
    }

    // 触发该定时器
    // LOG_DEBUG("[%s] timer[%d] triggerd.", LOG_TAG, node.id);
    node.cb();
    delete_(0);
  }
}

int Timer::get_next_timeout() {
  tick();

  
  size_t res = -1;
  if (!heap_.empty()) {
    auto expires = heap_.front().expires;
    res = std::chrono::duration_cast<MS>(expires - Clock::now()).count();
  }

  return res;
}

// 私有函数

void Timer::delete_(size_t i) {
  assert(!heap_.empty() && i >= 0 && i < heap_.size());
  size_t n = heap_.size() - 1;

  // 将该位置元素直接换到最后
  // 再调整原堆尾元素位置
  if (i < n) {
    swap_(i, n);
    if (!siftdown_(i, n)) {
      siftup_(i);
    }
  }

  // 删除堆尾元素
  timer_ref_.erase(heap_.back().id);
  heap_.pop_back();
}

void Timer::siftup_(size_t i) {
  assert(i >= 0 && i < heap_.size());
  // 二叉树中的父节点
  size_t j = (i - 1) / 2;

  while (j >= 0) {
    // 父节点小于当前节点,停止传递
    if (heap_[j] < heap_[i]) break;
    swap_(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

bool Timer::siftdown_(size_t i, size_t n) {
  assert(i >= 0 && i < heap_.size());
  assert(n >= 0 && n <= heap_.size());
  int init_pos = i;
  // 二叉树中的左子结点
  size_t j = i * 2 + 1;

  while (j < n) {
    // 查找子结点中较小的(否则交换后新节点不满足小于子结点的要求)
    if (j + 1 < n && heap_[j + 1] < heap_[j]) j++;
    // 父节点小于当前节点,停止传递
    if (heap_[i] < heap_[j]) break;
    swap_(i, j);
    i = j;
    j = i * 2 + 1;
  }

  // 返回是否进行过交换
  return i > init_pos;
}

void Timer::swap_(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());
  std::swap(heap_[i], heap_[j]);
  timer_ref_[heap_[i].id] = i;
  timer_ref_[heap_[j].id] = j;
}

}  // namespace TimelineServer
