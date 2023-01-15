#include "pool/sqlconnpool.h"

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

TEST(SQLConnPool, basic) {
  int max_conns = 10;

  Log::get_instance()->init(LOG_LEVEL::ELL_INFO, "../data/sqlconnpool_test_log", ".log", 0);

  SQLConnPool* pool = SQLConnPool::get_instance();
  pool->init("localhost", 3306, "root", "explosion", "timelineserver", max_conns);

  std::vector<sql::Connection*> conns;

  for (int i = 0; i < max_conns + 3; i++) {
    auto conn = pool->get_connect();
    if (conn != nullptr) conns.push_back(conn);
  }

  while (!conns.empty()) {
    auto conn = conns.back();
    conns.pop_back();

    pool->free_connect(conn);
  }
}

}  // namespace TimelineServer