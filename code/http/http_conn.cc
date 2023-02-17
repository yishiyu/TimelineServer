#include "http_conn.h"

const static char LOG_TAG[] = "HTTP_CONN";

namespace TimelineServer {

bool HttpConn::is_ET_;
string HttpConn::src_dir_;
std::atomic<int> HttpConn::user_count_;

HttpConn::HttpConn() {
  sock_fd_ = -1;
  sock_addr_ = {0};
  is_closed_ = true;
}

HttpConn::~HttpConn() { close_conn(); }

void HttpConn::init(int sock_fd, const sockaddr_in& sock_addr) {
  assert(sock_fd > 0);
  // 设置状态
  HttpConn::user_count_++;
  sock_fd_ = sock_fd;
  sock_addr_ = sock_addr;
  is_closed_ = false;
  // 清理缓存
  read_buff_.clear();
  write_buff_.clear();
  request_.clear();
  // LOG
  LOG_INFO("[%s] Client[%d](%s:%d) in, userCount:%d", LOG_TAG, sock_fd_,
           get_ip().data(), get_port(), (int)user_count_);
}

void HttpConn::close_conn() {
  // 直接命名为close会屏蔽掉标准库的close函数...
  if (false == is_closed_) {
    // 设置状态
    is_closed_ = true;
    HttpConn::user_count_--;
    // 关闭资源
    close(sock_fd_);
    response_.unmap_file();
    // 清理缓存
    read_buff_.clear();
    write_buff_.clear();
    // LOG
    LOG_INFO("[%s] Client[%d](%s:%d) quit, UserCount:%d", LOG_TAG, sock_fd_,
             get_ip().data(), get_port(), (int)user_count_);
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
    // 不能指望在没有内容读的时候 len=0
    // 其实这个时候 len = readv() = -1, errno = EAGAIN
    if (len <= 0) {
      return len;
    }
  } while (true);
}

ssize_t HttpConn::write(int* errno_) {
  ssize_t len = -1;

  do {
    len = writev(sock_fd_, iov_, iov_count_);
    // 和 read 一样,不能指望在没东西可写的时候 len=0
    // 应该根据 get_writable_bytes() 判断是否成功
    if (len <= 0) {
      *errno_ = errno;
      return len;
    }

    if (static_cast<size_t>(len) > iov_[0].iov_len) {
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
}

bool HttpConn::process() {
  request_.init();
  if (read_buff_.get_readable_bytes() <= 0) {
    return false;
  }

  // 解析报文
  auto result = request_.parse(read_buff_);
  if (result == HttpRequest::PARSE_RESULT::PR_SUCCESS) {
    // 返回成功报文
    response_.init(src_dir_, request_.get_path(), request_.get_is_keep_alive(),
                   200);
  } else if (result == HttpRequest::PARSE_RESULT::PR_INCOMPLETE) {
    // 报文体未接收完毕
    LOG_DEBUG("[%s] Try to continue to receive.", LOG_TAG)
    return false;
  } else {
    // 返回失败报文
    assert(result == HttpRequest::PARSE_RESULT::PR_ERROR);
    response_.init(src_dir_, request_.get_path(), false, 500);
  }

  // 响应报文
  write_buff_.clear();
  response_.make_response(this->request_, write_buff_);
  iov_[0].iov_base = const_cast<char*>(write_buff_.get_read_ptr());
  iov_[0].iov_len = write_buff_.get_readable_bytes();
  iov_count_ = 1;

  if (response_.get_file_size() > 0) {
    iov_[1].iov_base = response_.get_file();
    iov_[1].iov_len = response_.get_file_size();
    iov_count_ = 2;
  } else {
    iov_[1].iov_base = nullptr;
    iov_[1].iov_len = 0;
    iov_count_ = 1;
  }

  LOG_DEBUG("[%s] filesize:%d, response size:%d", LOG_TAG,
            response_.get_file_size(), get_writable_bytes());
  return true;
}

}  // namespace TimelineServer
