#include "http_request.h"

const static char LOG_TAG[] = "HTTP_REQUEST";

namespace TimelineServer {

void HttpRequest::init() {
  // 如果是解析到了 BODY 部分,则继续解析,而不是清空
  if (state_ != PARSE_STATE::PS_BODY) {
    state_ = PARSE_STATE::PS_REQUEST_LINE;
    method_ = "";
    path_ = "";
    version_ = "";
    body_ = "";

    header_.clear();
    post_ = Json();
  }
}

void HttpRequest::clear() {
  state_ = PARSE_STATE::PS_REQUEST_LINE;
  method_ = "";
  path_ = "";
  version_ = "";
  body_ = "";

  header_.clear();
  post_ = Json();
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

const Json HttpRequest::query_post(const string& key) const {
  assert(key != "");
  return post_[key];
}

const Json HttpRequest::query_post(const char* key) const {
  assert(key != nullptr);
  return post_[string(key)];
}

HttpRequest::PARSE_RESULT HttpRequest::parse(Buffer& buffer) {
  const char CRLF[] = "\r\n";
  if (buffer.get_readable_bytes() <= 0) {
    return PARSE_RESULT::PR_ERROR;
  }

  // string temp = buffer.read_all();
  // buffer.clear();
  // buffer.write_buffer(temp);
  // LOG_DEBUG("[%s]\r\n %s", LOG_TAG, temp.data());

  string line;
  const char* begin_ptr = buffer.get_read_ptr();
  const char* end_ptr = buffer.get_write_ptr();

  while (end_ptr >= begin_ptr) {
    const char* line_end = nullptr;
    if (state_ != PARSE_STATE::PS_BODY) {
      // 解析到下一个换行符
      line_end = std::search(begin_ptr, end_ptr, CRLF, CRLF + 2);
      line = string(const_cast<const char*>(begin_ptr), line_end);
    } else {
      // 解析剩下所有内容
      line_end = end_ptr;
      line = string(const_cast<const char*>(begin_ptr), line_end);

      {  // 检查报文体是否接收完毕
        int content_length = -1;
        if (header_.count("Content-Length") != 0) {
          // 实际收到的请求后面多了两个字符
          content_length = std::stoi(header_["Content-Length"]) + 2;
        } else if (header_.count("content-length") != 0) {
          // 实际收到的请求后面多了两个字符
          content_length = std::stoi(header_["content-length"]) + 2;
        } else {
          // POST 请求中没有写明请求体长度
          LOG_ERROR("[%s] Missing key: Content-Length", LOG_TAG);
          state_ = PARSE_STATE::PS_ERROR;
          return PARSE_RESULT::PR_ERROR;
        }

        LOG_DEBUG("[%s] Content Length:%d/%d", LOG_TAG, line.size(),
                  content_length);
        if (content_length > line.size()) {
          LOG_DEBUG("[%s] incomplete post data.", LOG_TAG);
          return PARSE_RESULT::PR_INCOMPLETE;
        } else if (content_length < line.size()) {
          LOG_ERROR("[%s] Error while parsing, unexpected data.", LOG_TAG);
          state_ = PARSE_STATE::PS_ERROR;
          return PARSE_RESULT::PR_ERROR;
        }

        // 未接收完毕的话,接收完毕后再次解析依然是从 BODY 继续
      }
    }

    switch (state_) {
      case PARSE_STATE::PS_REQUEST_LINE:
        state_ = parse_request_line_(line);
        break;

      case PARSE_STATE::PS_HEADERS:
        state_ = parse_header_(line);
        break;

      case PARSE_STATE::PS_BODY:
        state_ = parse_body_(line);
        break;

      case PARSE_STATE::PS_FINISH:
        return PARSE_RESULT::PR_SUCCESS;

      case PARSE_STATE::PS_ERROR:
        return PARSE_RESULT::PR_ERROR;

      default:
        break;
    }

    // 解析体可能由于报文未接受完毕而导致无法解析
    // 应该保留起来,等读完后续报文后再解析
    if (state_ != PARSE_STATE::PS_BODY) {
      // +2是额外的换行符
      buffer.move_read_ptr(line_end - begin_ptr + 2);
      begin_ptr = line_end + 2;
    }
  }

  // 正常情况下不应该读到这里
  // LOG_ERROR("[%s] Error while parsing, unexpected EOF", LOG_TAG);
  return PARSE_RESULT::PR_SUCCESS;
}

HttpRequest::PARSE_STATE HttpRequest::parse_request_line_(const string& line) {
  // GET /index HTTP/1.1
  std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch sub_match;

  if (std::regex_match(line, sub_match, pattern)) {
    method_ = sub_match[1];
    path_ = sub_match[2];
    version_ = sub_match[3];
    return PARSE_STATE::PS_HEADERS;
  }

  LOG_ERROR("[%s] Parse request line error!", LOG_TAG);
  return PARSE_STATE::PS_ERROR;
}

HttpRequest::PARSE_STATE HttpRequest::parse_header_(const string& line) {
  // Host: www.zhihu.com
  // User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0)
  // Connection: keep-alive
  // ...

  // 解析到空行
  if (line.size() == 0) {
    if (method_ == "POST") {
      return PARSE_STATE::PS_BODY;
    } else {
      return PARSE_STATE::PS_FINISH;
    }
  }

  // 解析请求头
  std::regex pattern_header("^([^:]*): ?(.*)$");
  std::smatch match_header;
  if (std::regex_match(line, match_header, pattern_header)) {
    header_[match_header[1]] = match_header[2];
    return PARSE_STATE::PS_HEADERS;
  }

  // 解析错误
  return PARSE_STATE::PS_ERROR;
}

HttpRequest::PARSE_STATE HttpRequest::parse_body_(const string& line) {
  if (header_["Content-Type"] == "application/json" ||
      header_["content-type"] == "application/json") {
    // 只解析json格式请求
    string error;
    post_ = Json::parse(line, error);

    if (error != "") {
      // 解析发生错误
      LOG_ERROR("[%s] Body(json) parse error: %s", LOG_TAG, error.data());
      return PARSE_STATE::PS_ERROR;
    }
  }

  return PARSE_STATE::PS_FINISH;
}

}  // namespace TimelineServer