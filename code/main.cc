#include <string>

#include "server/server.h"
using std::string;

int main() {
  string sql_host = "localhost";
  string sql_user = "root";
  string sql_pwd = "explosion";
  string sql_db_name = "timelineserver";
  string src_dir = "/home/yishiyu/Code/TimelineServer/";

  TimelineServer::Server server(2333, true, 60000, true, src_dir, sql_host,
                                3306, sql_user, sql_pwd, sql_db_name, 1, 1,
                                TimelineServer::LOG_LEVEL::ELL_DEBUG, 0);
  server.start();
}
