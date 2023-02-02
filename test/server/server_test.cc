#include "server/server.h"

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

TEST(Server, all) {
  string sql_host = "localhost";
  string sql_user = "root";
  string sql_pwd = "explosion";
  string sql_db_name = "timelineserver";
  string src_dir = "../";
  Server server(2333, true, 60000, true, src_dir, sql_host, 3306, sql_user,
                sql_pwd, sql_db_name, 1, 1, LOG_LEVEL::ELL_DEBUG, 0);

  server.start();
}

}  // namespace TimelineServer