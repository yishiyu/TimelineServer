#include <cppconn/prepared_statement.h>

#include <json11.hpp>
#include <string>

#include "buffer/buffer.h"
#include "http/http_request.h"
#include "log/log.h"
#include "pool/sql_conn_pool.h"
#include "server/server.h"

using std::string;
using TimelineServer::Buffer;
using TimelineServer::HttpRequest;

bool router_login(const HttpRequest& request, Buffer& buffer);
bool router_logout(const HttpRequest& request, Buffer& buffer);
bool router_query(const HttpRequest& request, Buffer& buffer);
bool router_add(const HttpRequest& request, Buffer& buffer);
bool router_modify(const HttpRequest& request, Buffer& buffer);
bool router_delete(const HttpRequest& request, Buffer& buffer);

unordered_map<string, string> action_tokens;
unordered_map<string, string> online_lists;

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
  server.register_dynamic_router("/action/modify", router_modify);
  server.register_dynamic_router("/action/delete", router_delete);

  server.start();
}

string token_generator(int length) {
  static string charset =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
  string result;
  result.resize(length);

  for (int i = 0; i < length; i++)
    result[i] = charset[rand() % charset.length()];

  return result;
}

bool router_login(const HttpRequest& request, Buffer& buffer) {
  Json::object result;

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    result["action_result"] = false;
    buffer.write_buffer(((Json)result).dump());
    LOG_INFO("Get SQL connection failed!");
    return true;
  }

  // 准备查询语句
  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement(
          "select * from users where user_name=? and user_passwd=?"));
  string user_name = request.query_post("action_info")["name"].string_value();
  string user_passwd =
      request.query_post("action_info")["passwd"].string_value();
  sql_pstmt->setString(1, user_name);
  sql_pstmt->setString(2, user_passwd);

  // 查询结果
  std::shared_ptr<sql::ResultSet> sql_result(sql_pstmt->executeQuery());
  assert(sql_result->rowsCount() < 2);
  if (sql_result->rowsCount() == 0) {
    result["action_result"] = false;
    buffer.write_buffer(((Json)result).dump());
    LOG_INFO("User[%s] login failed!", user_name.data());
    return true;
  }

  // 为该连接创建 action_token
  string action_token = token_generator(20);

  // 记录用户登录状态
  sql_result->next();
  action_tokens[action_token] = sql_result->getUInt64("user_id");
  online_lists[sql_result->getString("user_name")] = action_token;

  result["action_result"] = true;
  result["action_info"] = Json::object{{"action_token", action_token}};

  buffer.write_buffer(((Json)result).dump());
  LOG_INFO("User[%s] login success!", user_name.data());
  return true;
}

bool router_logout(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("logout");
  return true;
}

bool router_query(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("query");
  return true;
}

bool router_add(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("add");
  return true;
}

bool router_modify(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("modify");
  return true;
}

bool router_delete(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("delete");
  return true;
}