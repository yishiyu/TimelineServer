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

bool test_router(const HttpRequest& request, Buffer& buff) {
  string user_agent = request.query_header("User-Agent");
  buff.write_buffer("{\"result\": \"success\",\"User-Agent\": \"" + user_agent +
                    "\"}");

  return true;
}

TEST(Server, all) {
  TimelineServer::Server server(2333, true, 60000, true, "../", "localhost",
                                3306, "root", "explosion", "timelineserver", 1,
                                1, TimelineServer::LOG_LEVEL::ELL_DEBUG, 0);

  // server
  string router_url = "/test_action";
  server.register_dynamic_router(router_url, test_router);

  server.start();
}

}  // namespace TimelineServer