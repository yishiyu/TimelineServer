#include "http_request.h"

namespace TimelineServer {

const std::unordered_set<string> HttpRequest::ROUTER = {
    "/index",
    "/welcome",
};

void HttpRequest::init() {
  state_ = PARSE_STATE::REQUEST_LINE;
  method_ = "";
  path_ = "";
  version_ = "";
  body_ = "";

  header_.clear();
}

bool HttpRequest::get_is_keep_alive() const {
  if (header_.count("Connection") == 1) {
    return header_.find("Connection")->second == "keep-alive";
  }
  return false;
}

const string HttpRequest::query_header(const string& key) const {
  assert(key != "");
  if (header_.count(key) == 1) {
    return header_.find(key)->second;
  }
  return "";
}

const string HttpRequest::query_header(const char* key) const {
  assert(key != nullptr);
  if (header_.count(key) == 1) {
    return header_.find(key)->second;
  }
  return "";
}

bool HttpRequest::parse(Buffer& buffer) {
  const char CRLF[] = "\n";
  if (buffer.get_readable_bytes() <= 0) {
    return false;
  }

  while (buffer.get_readable_bytes() && state_ != PARSE_STATE::FINISH) {
    // 寻找换行符
    const char* line_end = std::search(buffer.get_read_ptr(),
                                       buffer.get_write_ptr(), CRLF, CRLF + 1);
    string line(const_cast<const char*>(buffer.get_read_ptr()), line_end);

    switch (state_) {
      case PARSE_STATE::REQUEST_LINE:
        if (!parse_request_line_(line)) {
          state_ = PARSE_STATE::ERROR;
          return false;
        }
        // 成功解析后解析请求文件路径
        path_decorate_();
        state_ = PARSE_STATE::HEADERS;
        break;

      case PARSE_STATE::HEADERS:
        if (!parse_header_(line)) {
          state_ = PARSE_STATE::BODY;
        }
        if (buffer.get_readable_bytes() <= 2) {
          // Get请求可能没有请求体,直接结束
          state_ = PARSE_STATE::FINISH;
        }
        break;

      case PARSE_STATE::BODY:
        if (!parse_body_(line)) {
          state_ = PARSE_STATE::FINISH;
        }
        break;

      default:
        break;
    }
    // +1是额外的换行符
    buffer.move_read_ptr(line.size() + 1);
  }

  return true;
}

bool HttpRequest::parse_request_line_(const string& line) {
  // GET /index HTTP/1.1
  std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch sub_match;

  if (std::regex_match(line, sub_match, pattern)) {
    method_ = sub_match[1];
    path_ = sub_match[2];
    version_ = sub_match[3];
    return true;
  }

  LOG_ERROR("Parse request line error!")
  return false;
}

bool HttpRequest::parse_header_(const string& line) {
  // Host: www.zhihu.com
  // User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0)
  // Connection: keep-alive
  // ...
  std::regex pattern("^([^:]*): ?(.*)$");
  std::smatch sub_match;

  if (std::regex_match(line, sub_match, pattern)) {
    header_[sub_match[1]] = sub_match[2];
    return true;
  }

  // 解析到空行或发生错误(总之匹配到意料之外的内容都当作解析到body起始标志处理)
  return false;
}

bool HttpRequest::parse_body_(const string& line) {
  // TODO 解析body,如json

  // 解析结束或发生错误
  return false;
}

void HttpRequest::path_decorate_() {
  // 为默认路径添加html后缀
  // 如 index, login
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (auto& item : ROUTER) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

}  // namespace TimelineServer