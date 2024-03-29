#include "routers.h"

unordered_map<uint64_t, string> online_lists_id2token;
unordered_map<string, uint64_t> online_lists_token2id;

const static char LOG_TAG[] = "ROUTERS";

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

  string user_name = request.query_post("action_info")["user"].string_value();
  string user_passwd =
      request.query_post("action_info")["passwd"].string_value();

  // 检查用户名/密码
  if (user_name.empty() || user_passwd.empty()) {
    LOG_DEBUG("[%s] Empty Username or Empty Passwd!", LOG_TAG);

    result["action_result"] = false;
    result["result_info"] =
        Json::object{{"error_info", "Empty Username or Empty Passwd!"}};
    buffer.write_buffer(((Json)result).dump());
    LOG_DEBUG("[%s] User[%s] login failed because of wrong password!", LOG_TAG,
              user_name.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("[%s] Get SQL connection failed!", LOG_TAG);
    // 直接返回 502 状态码
    return false;
  }

  // 准备查询语句
  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement(
          "select * from users where user_name=?"));

  sql_pstmt->setString(1, user_name);

  // 查询结果
  uint64_t user_id = 0;
  std::shared_ptr<sql::ResultSet> sql_result(sql_pstmt->executeQuery());
  if (sql_result->rowsCount() > 1) {
    LOG_WARN("[%s] 存在重名用户: %s", LOG_TAG, user_name.data());
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

    sql_pstmt_create_user->execute();
    LOG_DEBUG("[%s], User[%s] created successfully!", LOG_TAG,
              user_name.data());

    try {
      std::shared_ptr<sql::PreparedStatement> sql_pstmt_get_id(
          conn.connection->prepareStatement(
              "select last_insert_id() as user_id;"));

      std::shared_ptr<sql::ResultSet> sql_result_get_id(
          sql_pstmt_get_id->executeQuery());

      sql_result_get_id->next();
      user_id = sql_result_get_id->getUInt64("user_id");
    } catch (const sql::SQLException& e) {
      LOG_ERROR("[%s] SQLException: %s", LOG_TAG, e.what());
      return false;
    } catch (const std::exception& e) {
      LOG_ERROR("[%s] Exception: %s", LOG_TAG, e.what());
      return false;
    }

  } else {
    sql_result->next();
    // 用户存在,验证密码
    string temp = sql_result->getString("user_passwd").asStdString();
    if (temp == user_passwd) {
      // 用户验证成功
      LOG_DEBUG("User[%s] login successfully!", user_name.data());
      user_id = sql_result->getUInt64("user_id");
    } else {
      // 用户验证失败
      result["action_result"] = false;
      result["result_info"] = Json::object{{"error_info", "Wrong password!"}};
      buffer.write_buffer(((Json)result).dump());
      LOG_DEBUG("[%s] User[%s] login failed because of wrong password!",
                LOG_TAG, user_name.data());
      return true;
    }
  }

  // 为该连接创建 action_token
  string action_token = token_generator(20);

  // 记录用户登录状态

  online_lists_token2id[action_token] = user_id;
  online_lists_id2token[user_id] = action_token;

  result["action_result"] = true;
  result["result_info"] = Json::object{{"action_token", action_token}};
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User[%s] login successfully!", LOG_TAG, user_name.data());
  return true;
}

bool router_logout(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "Not logged in"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("[%s] User(action_token:[%s]) logout failed!", LOG_TAG,
              action_token.data());
    return true;
  }

  uint64_t user_id = online_lists_token2id[action_token];
  online_lists_id2token.erase(user_id);
  online_lists_token2id.erase(action_token);

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User(action_token:[%s]) logout successfully!", LOG_TAG,
            action_token.data());
  return true;
}

bool router_query(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "Not logged in"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("[%s] User(action_token:[%s]) query failed!(not logged in)",
              LOG_TAG, action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("[%s] Get SQL connection failed!", LOG_TAG);
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
  result["result_info"] = Json::object{{"tasks", Json(tasks)}};
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User[id:%d] query successfully!", LOG_TAG, user_id);
  return true;
}

bool router_add(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "Not logged in"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("[%s] User(action_token:[%s]) add failed!(not logged in)",
              LOG_TAG, action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("[%s] Get SQL connection failed!", LOG_TAG);
    // 直接返回 502 状态码
    return false;
  }

  // 准备add语句
  uint64_t user_id = online_lists_token2id[action_token];
  string time = request.query_post("action_info")["time"].string_value();
  string task = request.query_post("action_info")["task"].string_value();
  int priority = atoi(request.query_post("action_info")["priority"].string_value().data());

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
  try {
    sql_pstmt->execute();
  } catch (const sql::SQLException& e) {
    LOG_ERROR("[%s] SQLException: %s", LOG_TAG, e.what());
    return false;
  } catch (const std::exception& e) {
    LOG_ERROR("[%s] Exception: %s", LOG_TAG, e.what());
    return false;
  }

  try {
    std::shared_ptr<sql::PreparedStatement> sql_pstmt_get_id(
        conn.connection->prepareStatement(
            "select last_insert_id() as task_id;"));

    std::shared_ptr<sql::ResultSet> sql_result(
        sql_pstmt_get_id->executeQuery());

    sql_result->next();
    result["result_info"] = Json::object{
        {"task_id", sql_result->getString("task_id").asStdString()}};
  } catch (const sql::SQLException& e) {
    LOG_ERROR("[%s] SQLException: %s", LOG_TAG, e.what());
    return false;
  } catch (const std::exception& e) {
    LOG_ERROR("[%s] Exception: %s", LOG_TAG, e.what());
    return false;
  }

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User[id:%d] insert successfully!", LOG_TAG, user_id);
  return true;
}

bool router_update(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "Not logged in"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("[%s] User(action_token:[%s]) update failed!(not logged in)",
              LOG_TAG, action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("[%s] Get SQL connection failed!", LOG_TAG);
    // 直接返回 502 状态码
    return false;
  }

  // 准备update语句
  uint64_t task_id =
      std::stoi(request.query_post("action_info")["task_id"].string_value());
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

  int update_count = 0;
  try {
    update_count = sql_pstmt->executeUpdate();
  } catch (const sql::SQLException& e) {
    LOG_ERROR("[%s] SQLException: %s", LOG_TAG, e.what());
    return false;
  } catch (const std::exception& e) {
    LOG_ERROR("[%s] Exception: %s", LOG_TAG, e.what());
    return false;
  }

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User[task_id:%d] update successfully!", LOG_TAG, task_id);
  return true;
}

bool router_delete(const HttpRequest& request, Buffer& buffer) {
  Json::object result;
  string action_token =
      request.query_post("action_info")["action_token"].string_value();
  if (online_lists_token2id.count(action_token) == 0) {
    // 用户未登录
    result["action_result"] = false;
    result["result_info"] = Json::object{{"error_info", "Not logged in"}};
    buffer.write_buffer(((Json)result).dump());

    LOG_DEBUG("[%s] User(action_token:[%s]) delete failed!(not logged in)",
              LOG_TAG, action_token.data());
    return true;
  }

  // 创建数据库连接
  TimelineServer::SQLConn conn;
  if (!conn.is_valid()) {
    LOG_INFO("[%s] Get SQL connection failed!", LOG_TAG);
    // 直接返回 502 状态码
    return false;
  }

  // 准备delete语句
  uint64_t user_id = online_lists_token2id[action_token];
  uint64_t task_id = std::stoi(request.query_post("action_info")["task_id"].string_value());

  std::shared_ptr<sql::PreparedStatement> sql_pstmt(
      conn.connection->prepareStatement(
          "delete from tasks where task_id=? and user_id=?"));

  sql_pstmt->setUInt64(1, task_id);
  sql_pstmt->setUInt64(2, user_id);
  LOG_DEBUG("[%s] Delete task[%d] which owned to user[%d]", LOG_TAG, task_id,
            user_id);

  sql_pstmt->execute();

  result["action_result"] = true;
  buffer.write_buffer(((Json)result).dump());

  LOG_DEBUG("[%s] User[task_id:%d] delete successfully!", LOG_TAG, task_id);
  return true;
}