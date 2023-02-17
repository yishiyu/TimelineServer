#pragma once
#include <errno.h>

#include <json11.hpp>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "buffer/buffer.h"
#include "log/log.h"

using json11::Json;
using std::string;

namespace TimelineServer {

class HttpRequest {
 public:
  // 解析请求的有限状态机
  enum PARSE_STATE {
    PS_REQUEST_LINE,
    PS_HEADERS,
    PS_BODY,
    PS_FINISH,
    PS_ERROR,
  };

  enum PARSE_RESULT {
    PR_ERROR,
    PR_INCOMPLETE,
    PR_SUCCESS,
  };

  HttpRequest() { init(); }
  ~HttpRequest() = default;

  void init();
  void clear();
  PARSE_RESULT parse(Buffer& buffer);

  string get_path() const { return path_; };
  string& get_path() { return path_; };
  string get_method() const { return method_; };
  string get_version() const { return version_; };

  bool get_is_keep_alive() const;

  const string query_header(const string& key) const;
  const string query_header(const char* key) const;
  const std::unordered_map<string, string> get_header() const {
    return header_;
  }

  const Json query_post(const string& key) const;
  const Json query_post(const char* key) const;
  const Json get_post() const {
    return post_;
  }

 private:
  // 解析HTTP请求
  PARSE_STATE parse_request_line_(const string& line);
  PARSE_STATE parse_header_(const string& line);
  PARSE_STATE parse_body_(const string& line);

  // 工具函数(路径补全/json解析)
  void path_complete_();

  // TODO: 用户验证
  // static bool user_verify(const string& name, const string& pwd, bool
  // is_login);

  PARSE_STATE state_;
  string method_;
  string path_;
  string version_;
  string body_;

  std::unordered_map<string, string> header_;
  Json post_;
};

}  // namespace TimelineServer