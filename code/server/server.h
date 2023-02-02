#pragma once

#include <unordered_map>

#include "http/http_conn.h"
#include "log/log.h"
#include "mux/mux.h"
#include "pool/sql_conn_pool.h"
#include "pool/thread_pool.h"
#include "timer/timer.h"

namespace TimelineServer {
class Server {
 public:
  Server(int port, bool is_ET, int timeout_ms, bool linger_close,
         string& src_dir, string& sql_host, int sql_port,
         const string& sql_user, const string& sql_pwd,
         const string& sql_db_name, int pool_sql_conn_num, int pool_thread_num,
         LOG_LEVEL log_level, int log_queue_size);

  ~Server();

  void start();

 private:
  // 初始化函数
  void init_event_mode_(bool is_ET);
  bool init_socket_();

  // 处理事件函数
  void deal_new_conn_();
  void deal_close_conn_(HttpConn& client);
  void deal_read_conn_(HttpConn& client);
  void deal_write_conn_(HttpConn& client);

  // 工具函数
  void send_error_(int fd, const string& message);
  void extent_time_(HttpConn& client);
  void set_fd_noblock(int fd);

  // 回调函数(实际工作函数)
  void on_read_(HttpConn& client);
  void on_write_(HttpConn& client);
  void on_progress_(HttpConn& client);

  static const int MAX_FD = 65535;
  string src_dir_;
  int port_;
  bool linger_close_;
  int timeout_ms_;
  bool is_close_;
  int listen_fd_;
  bool is_ET_;

  uint32_t listen_events_;
  uint32_t conn_events_;

  std::unique_ptr<Timer> timer_;
  std::unique_ptr<ThreadPool> thread_pool_;
  std::unique_ptr<Mux> mux_;
  std::unordered_map<int, HttpConn> connections_;
};

}  // namespace TimelineServer
