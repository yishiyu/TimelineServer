#pragma once

#include <assert.h>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <semaphore.h>

#include <mutex>
#include <string>
#include <vector>

#include "log/log.h"

namespace TimelineServer {

#define MAX_URL_SIZE 200

class SQLConnPool {
 public:
  // 单例模式
  static SQLConnPool* get_instance();

  sql::Connection* get_connect();
  void free_connect(sql::Connection* conn);
  int get_free_connect_count();

  void init(const char* host, int port, const char* user, const char* pwd,
            const char* db_name, int max_conn_count);

  void close();

 private:
  SQLConnPool();
  ~SQLConnPool();

  int MAX_CONN_;

  std::vector<sql::Connection*> connections_;
  std::mutex mtx_;
  sem_t sem_id_;
};

// SQLConn RAII wapper
class SQLConn {
 public:
  SQLConn() {
    pool_ = SQLConnPool::get_instance();
    connection = pool_->get_connect();
  }

  bool is_valid() { return (connection != nullptr); }

  ~SQLConn() {
    if (connection != nullptr) {
      pool_->free_connect(connection);
    }
  }

  sql::Connection* connection;

 private:
  SQLConnPool* pool_;
};

}  // namespace TimelineServer
