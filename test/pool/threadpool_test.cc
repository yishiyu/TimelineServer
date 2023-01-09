#include "pool/threadpool.h"

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

TEST(ThreadPool, add_task) {
  ThreadPool pool(20);

  for (int task_id = 0; task_id < 10; task_id++) {
    pool.add_task(
        [task_id] { cout << "hello world in task " << task_id << endl; });
  }

  // 输出语句可能是乱序执行的
  sleep(3);
}

}  // namespace TimelineServer