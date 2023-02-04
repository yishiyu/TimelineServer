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
    LOG_INFO("Get SQL connection failed!");
    // 直接返回 502 状态码
    return false;
  }

  // 准备查询语句
  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement(
          "select * from users where user_name=?"));
  string user_name = request.query_post("action_info")["user"].string_value();
  string user_passwd =
      request.query_post("action_info")["passwd"].string_value();
  sql_pstmt->setString(1, user_name);

  // 查询结果
  std::shared_ptr<sql::ResultSet> sql_result(sql_pstmt->executeQuery());
  if (sql_result->rowsCount() > 1) {
    LOG_WARN("存在重名用户: %s", user_name.data());
    // 返回 502
    return false;
  }

  if (sql_result->rowsCount() == 0) {
    // 没有该用户, 新建用户
    std::shared_ptr<sql::PreparedStatement> sql_pstmt_create_user(
        conn.connection->prepareStatement(
            "insert into users(user_name, user_passwd) values(?, ?)"));
    sql_pstmt_create_user->setString(1, user_name);
    sql_pstmt_create_user->setString(2, user_passwd);

    if (sql_pstmt_create_user->execute()) {
      LOG_DEBUG("User[%s] created successfully!", user_name.data());
      // 重新执行查询语句
      sql_result.reset(sql_pstmt->executeQuery());
    } else {
      // 创建用户失败
      result["action_result"] = false;
      result["result_info"] = Json::object{{"error_info", "创建用户失败"}};
      buffer.write_buffer(((Json)result).dump());

      LOG_DEBUG("Failed to create user[%s]!", user_name.data());
      return true;
    }

  } else {
    sql_result->next();
    // 用户存在,验证密码
    string temp = sql_result->getString("user_passwd").asStdString();
    if (temp == user_passwd) {
      // 用户验证成功
      // LOG_DEBUG("User[%s] login successfully!", user_name.data());
    } else {
      // 用户验证失败
      result["action_result"] = false;
      result["result_info"] = Json::object{{"error_info", "帐号密码错误"}};
      buffer.write_buffer(((Json)result).dump());
      LOG_DEBUG("User[%s] login failed because of wrong password!",
                user_name.data());
      return true;
    }
  }

  // 为该连接创建 action_token
  string action_token = token_generator(20);

  // 记录用户登录状态
  string user_id = std::to_string(sql_result->getUInt64("user_id"));
  action_tokens[action_token] = user_id;
  online_lists[user_name] = action_token;

  result["action_result"] = true;
  result["result_info"] = Json::object{{"action_token", action_token}};
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User[%s] login successfully!", user_name.data());
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