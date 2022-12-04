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
  Log::get_instance()->init(LOG_LEVEL::ELL_INFO, "../data/log", ".log", 0);

  LOG_INFO("%s", "============== sync_log =============")
  LOG_DEBUG("%s", "a debug message written by sync_log");
  LOG_INFO("%s", "an info message written by sync_log");
  LOG_WARN("%s", "a warning message written by sync_log");
  LOG_ERROR("%s", "an error message written by sync_log");
  LOG_INFO("%s", "(不应该记录 DEBUG 日志)");
  LOG_INFO("%s", "=====================================")
}

TEST(Log, async_log) {
  Log::get_instance()->init(LOG_LEVEL::ELL_DEBUG, "../data/log", ".log", 100);

  LOG_INFO("%s", "============= async_log =============")
  LOG_DEBUG("%s", "a debug message written by async_log");
  LOG_INFO("%s", "an info message written by async_log");
  LOG_WARN("%s", "a warning message written by async_log");
  LOG_ERROR("%s", "an error message written by async_log");
  LOG_INFO("%s", "(应该记录 DEBUG 日志)");
  LOG_INFO("%s", "=====================================")
}

}  // namespace TimelineServer