#include "routers.h"
#include "server/server.h"

int main() {
  TimelineServer::Server server(2333, true, 60000, true, "../", "localhost",
                                3306, "root", "explosion", "timelineserver", 1,
                                1, TimelineServer::LOG_LEVEL::ELL_DEBUG, 0);

  // 注册静态路由
  server.register_static_router("/", "/login.html");

  // 注册动态路由
  server.register_dynamic_router("/action/login", router_login);
  server.register_dynamic_router("/action/logout", router_logout);
  server.register_dynamic_router("/action/query", router_query);
  server.register_dynamic_router("/action/add", router_add);
  server.register_dynamic_router("/action/update", router_update);
  server.register_dynamic_router("/action/delete", router_delete);

  server.start();
}
