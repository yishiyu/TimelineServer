#include "server.h"

const static char LOG_TAG[] = "SERVER";

namespace TimelineServer {

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

Server::Server(int port, bool is_ET, int timeout_ms, bool linger_close,
               const char* src_dir, const char* sql_host, int sql_port,
               const char* sql_user, const char* sql_pwd,
               const char* sql_db_name, int pool_sql_conn_num,
               int pool_thread_num, LOG_LEVEL log_level, int log_queue_size)
    : port_(port),
      linger_close_(linger_close),
      timeout_ms_(timeout_ms),
      is_close_(false),
      timer_(new Timer()),
      thread_pool_(new ThreadPool(pool_thread_num)),
      mux_(new Mux()) {
  // 获取工作目录
  // char src_dir[256] = {0};
  // getcwd(src_dir, 256);
  src_dir_ = string(src_dir) + "data/resources";
  string log_dir = string(src_dir) + "data/log";

  Log::get_instance()->init(log_level, log_dir.data(), ".log", log_queue_size);

  // 初始化数据库
  SQLConnPool::get_instance()->init(sql_host, sql_port, sql_user, sql_pwd,
                                    sql_db_name, pool_sql_conn_num);

  // 初始化服务器
  init_event_mode_(is_ET);
  if (!init_socket_()) {
    is_close_ = true;
  }
  if (!init_quit_signal_()) {
    is_close_ = true;
  }

  // 初始化 Http 连接
  HttpConn::global_config(true, src_dir_, 0);

  // LOG
  {
    string log_level_stirng;
    switch (log_level) {
      case ELL_DEBUG:
        log_level_stirng = "DEBUG";
        break;
      case ELL_INFO:
        log_level_stirng = "ELL_INFO";
        break;
      case ELL_WARN:
        log_level_stirng = "ELL_WARN";
        break;
      case ELL_ERROR:
        log_level_stirng = "ELL_ERROR";
        break;
      default:
        LOG_ERROR("Log level error");
        is_close_ = false;
        break;
    }

    if (is_close_) {
      LOG_ERROR("[%s] ========== Server init error!==========", LOG_TAG);
    } else {
      LOG_INFO("[%s] ========== Server init ==========", LOG_TAG);
      LOG_INFO("[%s] Port:%d \tLinger close: %s", LOG_TAG, port_,
               linger_close_ ? "true" : "false");
      LOG_INFO("[%s] Triger mode: %s", LOG_TAG, (is_ET_ ? "ET" : "LT"));
      LOG_INFO("[%s] Log level: %s", LOG_TAG, log_level_stirng.data());
      LOG_INFO("[%s] Src dir: %s", LOG_TAG, src_dir_.data());
      LOG_INFO("[%s] SQL connection pool size: %d, Thread pool size: %d", LOG_TAG,
               pool_sql_conn_num, pool_thread_num);
    }
  }
}

Server::~Server() {
  close(listen_fd_);
  is_close_ = true;
  SQLConnPool::get_instance()->close();
  LOG_INFO("[%s] ========== Server stop ==========", LOG_TAG);
  LOG_INFO("[%s] Bye~", LOG_TAG)
}

void Server::start() {
  // time to next tick(MS)
  int ttnt_ms = -1;
  if (!is_close_) {
    LOG_INFO("[%s] ========== Server start ==========", LOG_TAG);
  }
  while (!is_close_) {
    if (timeout_ms_ > 0) {
      // get_next_timeout 函数内会执行 tick
      ttnt_ms = timer_->get_next_timeout();
    }
    int events_count_ = mux_->wait(ttnt_ms);
    for (int i = 0; i < events_count_; i++) {
      int fd = mux_->get_active_fd(i);
      uint32_t event = mux_->get_active_events(i);
      if (fd == listen_fd_) {
        // 有新连接
        deal_new_conn_();
      } else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        // 连接断开
        assert(connections_.count(fd) > 0);
        LOG_DEBUG("[%s] Connection[%d] disconnect event triggered.", LOG_TAG, fd);
        deal_close_conn_(connections_[fd]);
      } else if (event & EPOLLIN) {
        // 收到数据
        deal_read_conn_(connections_[fd]);
      } else if (event & EPOLLOUT) {
        // 发送数据
        deal_write_conn_(connections_[fd]);
      } else {
        LOG_WARN("[%s] Unexpected event: %d!", LOG_TAG, event);
      }
    }
  }
}

bool Server::register_static_router(string& src, string& des) {
  return HttpResponse::register_static_router(src, des);
}

bool Server::register_static_router(const char* src, string& des) {
  assert(src != nullptr);
  string src_(src);
  return HttpResponse::register_static_router(src_, des);
}

bool Server::register_static_router(string& src, const char* des) {
  assert(des != nullptr);
  string des_(des);
  return HttpResponse::register_static_router(src, des_);
}

bool Server::register_static_router(const char* src, const char* des) {
  assert(src != nullptr && des != nullptr);
  string src_(src);
  string des_(des);
  return HttpResponse::register_static_router(src_, des_);
}

bool Server::register_dynamic_router(string& src, const router_cb& cb) {
  return HttpResponse::register_dynamic_router(src, cb);
}

bool Server::register_dynamic_router(const char* src, const router_cb& cb) {
  assert(src != nullptr);
  string src_(src);
  return HttpResponse::register_dynamic_router(src_, cb);
}

void Server::init_event_mode_(bool is_ET) {
  // 边缘触发监听(监听连接到来,额外监听读取关闭)
  listen_events_ = EPOLLET | EPOLLIN | EPOLLRDHUP;

  // 对于客户端,oneshot形式监听(额外监听读取关闭)
  conn_events_ = EPOLLONESHOT | EPOLLRDHUP;
  if (is_ET) {
    conn_events_ = conn_events_ | EPOLLET;
  }
  is_ET_ = is_ET;
}

bool Server::init_socket_() {
  int ret = -1;

  // 1. 创建端口
  struct sockaddr_in addr;
  if (port_ > 65535 || port_ < 1024) {
    // 超出可用端口号范围
    LOG_ERROR("[%s] Port:%d error!", LOG_TAG, port_);
    return false;
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    LOG_ERROR("[%s] Create socket error, Port: %d", LOG_TAG, port_);
    return false;
  }

  // 2. 设置端口
  {
    // 设置优雅关闭
    struct linger opt_linger = {0};
    if (linger_close_) {
      // 优雅关闭,直到所剩数据发送完毕或超时
      opt_linger.l_onoff = 1;
      // Posix.1g 规定单位为秒
      opt_linger.l_linger = 1;
    }

    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger,
                     sizeof(opt_linger));
    if (ret < 0) {
      close(listen_fd_);
      LOG_ERROR("[%s] Init linger error!", LOG_TAG);
      return false;
    }
  }

  {
    int opt_val = 1;
    // SO_REUSEADDR:
    //   For AF_INET sockets this means that a socket
    //   may bind, except when there is an active listening socket
    //   bound to the address.  When the listening socket is bound
    //   to INADDR_ANY with a specific port then it is not possible
    //   to bind to this port for any local address.
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR,
                     (const void*)&opt_val, sizeof(int));
    if (ret == -1) {
      close(listen_fd_);
      LOG_ERROR("[%s] Set socket setsockopt error !", LOG_TAG);
      return false;
    }
  }

  // 3. 绑定端口
  ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("[%s] Bind port error,Port: %d", LOG_TAG, port_);
    return false;
  }

  // 4. 监听端口
  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("[%s] Listen port error,Port: %d", LOG_TAG, port_);
    return false;
  }

  // 5. 将监听的端口添加到 IO 复用中
  ret = mux_->add_fd(listen_fd_, listen_events_);
  if (ret == 0) {
    close(listen_fd_);
    LOG_ERROR("[%s] Add listened socket to mux error!", LOG_TAG);
    return false;
  }

  // 6. 设置被监听socket为非阻塞模式
  set_fd_noblock(listen_fd_);

  LOG_INFO("[%s] Server port:%d", LOG_TAG, port_);
  return true;
}

bool Server::init_quit_signal_() {
  // https://stackoverflow.com/questions/11468414/using-auto-and-lambda-to-handle-signal
  // 直接使用 lambda 函数会报类型转换错误
  // 可以用functional包装一下
  signal(SIGINT, signal_handler);
  shutdown_handler = [this](int sig) { this->is_close_ = true; };

  return true;
}

void Server::deal_new_conn_() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);
  if (fd < 0) {
    LOG_ERROR("[%s] New connection creation failed", LOG_TAG);
    return;
  }

  // 限制最大客户端数量
  if (HttpConn::get_user_count() >= MAX_FD) {
    LOG_WARN("[%s] Server busy!", LOG_TAG);
    send_error_(fd, "Server Busy!");
    return;
  }

  // 成功建立连接,将新连接加入管理列表
  connections_[fd].init(fd, addr);
  if (timeout_ms_ > 0) {
    timer_->add_timer(
        fd, timeout_ms_,
        std::bind(&Server::deal_close_conn_, this, std::ref(connections_[fd])));
  }

  set_fd_noblock(fd);
  mux_->add_fd(fd, conn_events_ | EPOLLIN);
  LOG_INFO("[%s] Client[%d] in!", LOG_TAG, fd);
}

void Server::deal_close_conn_(HttpConn& client) {
  if(client.is_closed()){
    LOG_WARN("[%s] Close a closed connection[%d].", LOG_TAG, client.get_fd());
    return;
  }
  LOG_INFO("[%s] Client[%d] quit!", LOG_TAG, client.get_fd());

  int ret = mux_->del_fd(client.get_fd());
  client.close_conn();
}

void Server::deal_read_conn_(HttpConn& client) {
  // 交给线程池异步处理
  extent_time_(client);
  thread_pool_->add_task(std::bind(&Server::on_read_, this, std::ref(client)));
}

void Server::deal_write_conn_(HttpConn& client) {
  // 交给线程池异步处理
  extent_time_(client);
  thread_pool_->add_task(std::bind(&Server::on_write_, this, std::ref(client)));
}

void Server::send_error_(int fd, const string& message) {
  int ret = send(fd, message.data(), message.size(), 0);
  if (ret < 0) {
    LOG_WARN("[%s] send error to client[%d] failed.", LOG_TAG, fd);
  }
  close(fd);
}

void Server::extent_time_(HttpConn& client) {
  if (timeout_ms_ > 0) {
    timer_->adjust(client.get_fd(), timeout_ms_);
  }
}

void Server::set_fd_noblock(int fd) {
  assert(fd > 0);
  // O_NONBLOCK:
  //    If a process tries to perform an
  //    incompatible access (e.g., read(2) or write(2)) on a file region
  //    that has an incompatible mandatory lock, then the result depends
  //    upon whether the O_NONBLOCK flag is enabled for its open file
  //    description.  If the O_NONBLOCK flag is not enabled, then the
  //    system call is blocked until the lock is removed or converted to
  //    a mode that is compatible with the access.  If the O_NONBLOCK
  //    flag is enabled, then the system call fails with the error
  //    EAGAIN.
  // 简而言之,设置了 O_NONBLOCK 之后,read 和 write
  // 遇到问题(如写缓冲区满等)后会直接返回 并设置 errno 为
  // EAGAIN(不设置的话会阻塞直到问题解决).
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, NULL) | O_NONBLOCK);
}

void Server::on_read_(HttpConn& client) {
  if (client.is_closed()){
    LOG_WARN("[%s] Read a closed connection[%d].", LOG_TAG, client.get_fd());
    return;
  }

  int errno_;
  int ret = client.read(&errno_);
  if (ret < 0) {
    // 最后一般是 ret = 0
    if (errno_ == EAGAIN) {
      // 暂时不可读,等待机会再读
      mux_->mod_fd(client.get_fd(), conn_events_ | EPOLLIN);
      return;
    } else {
      LOG_ERROR("[%s] Client[%d] read error with errno:%d", LOG_TAG, client.get_fd(), errno_);
      deal_close_conn_(client);
      return;
    }
  }

  // 在同一线程中继续处理报文
  on_progress_(client);
}

void Server::on_write_(HttpConn& client) {
  if (client.is_closed()){
    LOG_WARN("[%s] Write to a closed connection[%d].", LOG_TAG, client.get_fd());
    return;
  }

  int errno_;
  int ret = client.write(&errno_);
  if (ret < 0) {
    // 最后一般是 ret = 0
    if (errno_ == EAGAIN) {
      // 暂时不可写,等待机会再写
      mux_->mod_fd(client.get_fd(), conn_events_ | EPOLLOUT);
      return;
    } else {
      // 其他未知错误
      LOG_ERROR("[%s] Client[%d] write error with errno:%d", LOG_TAG, client.get_fd(),
                errno_);
      deal_close_conn_(client);
      return;
    }
  }

  // 传输完成
  if (client.is_keep_alive()) {
    mux_->mod_fd(client.get_fd(), conn_events_ | EPOLLIN);
  } else {
    deal_close_conn_(client);
  }
}

void Server::on_progress_(HttpConn& client) {
  if (client.is_closed()){
    LOG_WARN("[%s] Process a closed connection[%d].", LOG_TAG, client.get_fd());
    return;
  }

  if (client.process()) {
    // 处理报文成功,等待可写时回复
    mux_->mod_fd(client.get_fd(), conn_events_ | EPOLLOUT);
  } else {
    // 处理失败,等待重新接收报文
    mux_->mod_fd(client.get_fd(), conn_events_ | EPOLLIN);
  }
}

}  // namespace TimelineServer
