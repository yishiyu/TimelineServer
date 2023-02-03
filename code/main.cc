#include <json11.hpp>
#include <string>

#include "buffer/buffer.h"
#include "http/http_request.h"
#include "server/server.h"
using std::string;

bool router_login(const TimelineServer::HttpRequest& request,
                  TimelineServer::Buffer& buffer);
bool router_logout(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer);
bool router_query(const TimelineServer::HttpRequest& request,
                  TimelineServer::Buffer& buffer);
bool router_add(const TimelineServer::HttpRequest& request,
                TimelineServer::Buffer& buffer);
bool router_modify(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer);
bool router_delete(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer);

int main() {
  TimelineServer::Server server(2333, true, 60000, true, "./", "localhost",
                                3306, "root", "explosion", "timelineserver", 1,
                                1, TimelineServer::LOG_LEVEL::ELL_DEBUG, 0);

  // 注册静态路由
  server.register_static_router("/", "/login.html");

  // 注册动态路由
  server.register_dynamic_router("/action/login", router_login);
  server.register_dynamic_router("/action/logout", router_logout);
  server.register_dynamic_router("/action/query", router_query);
  server.register_dynamic_router("/action/add", router_add);
  server.register_dynamic_router("/action/modify", router_modify);
  server.register_dynamic_router("/action/delete", router_delete);

  server.start();
}

bool router_login(const TimelineServer::HttpRequest& request,
                  TimelineServer::Buffer& buffer) {
  buffer.write_buffer("login");
  return true;
}

bool router_logout(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer) {
  buffer.write_buffer("logout");
  return true;
}

bool router_query(const TimelineServer::HttpRequest& request,
                  TimelineServer::Buffer& buffer) {
  buffer.write_buffer("query");
  return true;
}

bool router_add(const TimelineServer::HttpRequest& request,
                TimelineServer::Buffer& buffer) {
  buffer.write_buffer("add");
  return true;
}

bool router_modify(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer) {
  buffer.write_buffer("modify");
  return true;
}

bool router_delete(const TimelineServer::HttpRequest& request,
                   TimelineServer::Buffer& buffer) {
  buffer.write_buffer("delete");
  return true;
}