#include "timer/timer.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <iostream>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {

void hello() { cout << "hello world" << endl; }

TEST(Timer, add_timer) {
  Timer timer;
  timer_id id = 111;
  timer.add_timer(id, 3000, &hello);

  sleep(2);
  cout << "after 2 seconds" << endl;
  timer.tick();
  sleep(2);
  cout << "after 2 seconds" << endl;
  timer.tick();

  timer.clear();
}

}  // namespace TimelineServer