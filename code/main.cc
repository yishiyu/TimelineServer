#include <string>

#include "server/server.h"
using std::string;

int main() {
  TimelineServer::Server server(2333, true, 60000, true, "./", "localhost",
                                3306, "root", "explosion", "timelineserver", 1,
                                1, TimelineServer::LOG_LEVEL::ELL_DEBUG, 0);
  server.start();
}
