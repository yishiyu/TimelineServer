#include "http_conn.h"

namespace TimelineServer {

HttpConn::HttpConn() {
  sock_fd_ = -1;
  sock_addr_ = {0};
  is_closed_ = true;
}

HttpConn::~HttpConn() { close_conn(); }

void HttpConn::init(int sock_fd, const sockaddr_in& sock_addr) {
  assert(sock_fd > 0);
  // 设置状态
  user_count_++;
  sock_fd_ = sock_fd;
  sock_addr_ = sock_addr;
  is_closed_ = false;
  // 清理缓存
  read_buff_.clear();
  write_buff_.clear();
  // LOG
  LOG_INFO("Client[%d](%s:%d) in, userCount:%d", sock_fd_, get_ip().data(),
           get_port(), (int)user_count_);
}

void HttpConn::close_conn() {
  // 直接命名为close会屏蔽掉标准库的close函数...
  if (false == is_closed_) {
    // 设置状态
    is_closed_ = true;
    user_count_--;
    // 关闭资源
    close(sock_fd_);
    response_.unmap_file();
    // LOG
    LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", sock_fd_, get_ip().data(),
             get_port(), (int)user_count_);
  }
}

void HttpConn::global_config(bool is_ET, string& src_dir, int user_count) {
  HttpConn::is_ET_ = is_ET;
  HttpConn::src_dir_ = src_dir;
  HttpConn::user_count_ = user_count;
}

ssize_t HttpConn::read(int* errno_) {
  ssize_t len = -1;

  do {
    // 不检测是否读完消息体,再解析报文,可(ken)能(ding)有一些bug
    // 必须保证 HTTP 报文在读取的时候全部到达(但无法保证)
    len = read_buff_.read_fd(sock_fd_, errno_);
    if (len <= 0) {
      break;
    }
  } while (true);

  return len;
}

ssize_t HttpConn::write(int* errno_) {
  ssize_t len = -1;

  do {
    len = writev(sock_fd_, iov_, iov_count_);
    if (len <= 0) {
      *errno_ = errno;
      break;
    }

    if (get_writable_bytes() == 0) {
      // 传输完成
      break;
    } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      // 响应头传输完成
      iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      if (iov_[0].iov_len) {
        write_buff_.clear();
        iov_[0].iov_len = 0;
      }
    } else {
      iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
      iov_[0].iov_len -= len;
      write_buff_.move_read_ptr(len);
    }

  } while (true);

  return len;
}

bool HttpConn::process() {
  request_.init();
  if (read_buff_.get_readable_bytes() <= 0) {
    return false;
  }

  // 解析报文
  if (request_.parse(read_buff_)) {
    LOG_DEBUG("%s", request_.get_path().data());
    // 返回成功报文
    response_.init(src_dir_, request_.get_path(), request_.get_is_keep_alive(),
                   200);
  } else {
    // 返回失败报文
    response_.init(src_dir_, request_.get_path(), false, 400);
  }

  // 响应报文
  response_.make_response(write_buff_);
  iov_[0].iov_base = const_cast<char*>(write_buff_.get_read_ptr());
  iov_[0].iov_len = write_buff_.get_readable_bytes();
  iov_count_ = 1;

  if (response_.get_file_size() > 0) {
    iov_[1].iov_base = response_.get_file();
    iov_[1].iov_len = response_.get_file_size();
    iov_count_ = 2;
  }

  LOG_DEBUG("filesize:%d, %d  to %d", response_.get_file_size(), iov_count_,
            get_writable_bytes());
  return true;
}

}  // namespace TimelineServer
