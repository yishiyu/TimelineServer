#include "log/log.h"

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

TEST(Log, sync_log) {
  Log::get_instance()->init(LOG_LEVEL::ELL_DEBUG, "../data/log", ".log", 0);


  LOG_DEBUG("%s", "debug log");

  Log::get_instance()->flush();
}

}  // namespace TimelineServer