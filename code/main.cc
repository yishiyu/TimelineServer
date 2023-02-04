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
bool router_update(const HttpRequest& request, Buffer& buffer);
bool router_delete(const HttpRequest& request, Buffer& buffer);

unordered_map<uint64_t, string> online_lists_id2token;
unordered_map<string, uint64_t> online_lists_token2id;

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
  uint64_t user_id = sql_result->getUInt64("user_id");
  online_lists_token2id[action_token] = user_id;
  online_lists_id2token[user_id] = action_token;

  result["action_result"] = true;
  result["result_info"] = Json::object{{"action_token", action_token}};
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User[%s] login successfully!", user_name.data());
  return true;
}

bool router_logout(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "用户未登录"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("User(action_token:[%s]) logout failed!", action_token.data());
    return true;
  }

  uint64_t user_id = online_lists_token2id[action_token];
  online_lists_id2token.erase(user_id);
  online_lists_token2id.erase(action_token);

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User(action_token:[%s]) logout successfully!",
            action_token.data());
  return true;
}

struct task {
  string task_id_;
  string time_;
  string task_;
  string priority_;
  task(const string& task_id, const string& time, const string& task,
       const string& priority)
      : task_id_(task_id), time_(time), task_(task), priority_(priority) {}
  Json to_json() const {
    return Json::object{{"task_id", task_id_},
                        {"time", time_},
                        {"task", task_},
                        {"priority", priority_}};
  }
};

bool router_query(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "用户未登录"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("User(action_token:[%s]) logout failed!", action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("Get SQL connection failed!");
    // 直接返回 502 状态码
    return false;
  }

  // 准备查询语句
  uint64_t user_id = online_lists_token2id[action_token];
  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement("select * from tasks where user_id=?"));
  sql_pstmt->setUInt64(1, user_id);

  std::shared_ptr<sql::ResultSet> sql_result(sql_pstmt->executeQuery());

  std::vector<task> tasks;
  while (sql_result->next()) {
    tasks.push_back(task(sql_result->getString("task_id").asStdString(),
                         sql_result->getString("time").asStdString(),
                         sql_result->getString("task").asStdString(),
                         sql_result->getString("priority").asStdString()));
  }

  result["action_result"] = true;
  result["result_info"] =
      Json::object{{"result_info", Json::object{{"tasks", Json(tasks)}}}};
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User[id:%d] query successfully!", user_id);
  return true;
}

bool router_add(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "用户未登录"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("User(action_token:[%s]) logout failed!", action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("Get SQL connection failed!");
    // 直接返回 502 状态码
    return false;
  }

  // 准备add语句
  uint64_t user_id = online_lists_token2id[action_token];
  string time = request.query_post("action_info")["time"].string_value();
  string task = request.query_post("action_info")["task"].string_value();
  int priority = request.query_post("action_info")["priority"].int_value();

  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement("insert into tasks(user_id, time, "
                                        "task, priority) values(?, ?, ?, ?)"));

  sql_pstmt->setUInt64(1, user_id);
  sql_pstmt->setString(2, time);
  sql_pstmt->setString(3, task);
  sql_pstmt->setInt(4, priority);

  // 这个execute函数有点抽风,从结果看命名插入了但还是返回了false
  // 没找到C++的文档,Java的文档里是这么说的
  // true if the first result is a ResultSet object; false if it is an update
  // count or there are no results 先不用这个返回值判断是否成功插入
  sql_pstmt->execute();

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User[id:%d] insert successfully!", user_id);
  return true;
}

bool router_update(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "用户未登录"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("User(action_token:[%s]) logout failed!", action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("Get SQL connection failed!");
    // 直接返回 502 状态码
    return false;
  }

  // 准备update语句
  uint64_t task_id = request.query_post("action_info")["task_id"].int_value();
  string time = request.query_post("action_info")["time"].string_value();
  string task = request.query_post("action_info")["task"].string_value();
  int priority = request.query_post("action_info")["priority"].int_value();

  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement(
          "update tasks set time=?, task=?, priority=? where task_id=?"));

  sql_pstmt->setString(1, time);
  sql_pstmt->setString(2, task);
  sql_pstmt->setInt(3, priority);
  sql_pstmt->setUInt64(4, task_id);

  int update_count = sql_pstmt->executeUpdate();

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("User[task_id:%d] update successfully!", task_id);
  return true;
}

bool router_delete(const HttpRequest& request, Buffer& buffer) {
  buffer.write_buffer("delete");
  return true;
}