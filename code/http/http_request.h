#pragma once
#include <errno.h>

#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "buffer/buffer.h"
#include "log/log.h"

using std::string;

namespace TimelineServer {

class HttpRequest {
 public:
  // 解析请求的有限状态机
  enum PARSE_STATE {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH,
    ERROR,
  };

  HttpRequest() { init(); }
  ~HttpRequest() = default;

  void init();
  bool parse(Buffer& buffer);

  string get_path() const { return path_; };
  string& get_path() { return path_; };
  string get_method() const { return method_; };
  string get_version() const { return version_; };
  bool is_keep_alive() const { return is_keep_alive_; };

  // TODO: 解析Post请求
  // string get_post(const string& key) const;
  // string get_post(const char* key) const;

 private:
  // 解析HTTP请求
  bool parse_request_line_(const string& line);
  bool parse_header_(const string& line);
  bool parse_body_(const string& line);

  // 对解析结果进行额外处理
  void path_decorate_();

  // TODO: 解析Post请求
  // void parse_post_();

  // TODO: 用户验证
  // static bool user_verify(const string& name, const string& pwd, bool
  // is_login);

  PARSE_STATE state_;
  string method_;
  string path_;
  string version_;
  string body_;
  bool is_keep_alive_;

  std::unordered_map<string, string> header_;
  // std::unordered_map<string, string> post_;

  static const std::unordered_set<string> ROUTER;
};

}  // namespace TimelineServer