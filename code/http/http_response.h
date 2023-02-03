#pragma once

#include <sys/mman.h>
#include <sys/stat.h>

#include <functional>
#include <string>
#include <unordered_map>

#include "buffer/buffer.h"
#include "log/log.h"
using std::string;
using std::unordered_map;

namespace TimelineServer {

// 动态路由回调函数
typedef std::function<bool(Buffer& buffer)> router_cb;

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void init(const std::string& src_dir, const std::string& file_path,
            bool is_keep_alive = false, int code = -1);

  void make_response(Buffer& buffer);

  // 获取映射到内存的文件/回收内存文件
  char* get_file();
  void unmap_file();

  size_t get_file_size() const { return mm_file_stat_.st_size; };
  int get_code() const { return code_; };

  static bool register_static_router(string& src, string& des);
  static bool register_dynamic_router(string& src, const router_cb& cb);

 private:
  // 填充响应消息
  void add_state_line_(Buffer& buff);
  void add_header_(Buffer& buff);
  void add_content_(Buffer& buff);
  // 填充信息时发生错误
  void add_error_content(Buffer& buff, std::string message);

  // 响应状态码
  void response_to_code_();

  // 获取请求文件类型(根据文件后缀)
  string get_file_type_();

  // 动态响应 Buffer
  Buffer dynamic_buffer_;

  // 状态码等
  int code_;
  bool is_keep_alive_;

  // 路径和文件名
  string src_dir_;
  string file_path_;

  // 映射到内存的文件
  char* mm_file_;
  struct stat mm_file_stat_;

  static const unordered_map<string, string> SUFFIX_TYPE;
  static const unordered_map<int, string> CODE_STATUS;
  static const unordered_map<int, string> ERROR_CODE_FILE;

  // 动态路由负责需要查询数据库的POST请求, 静态路由负责静态资源文件跳转
  static unordered_map<string, router_cb> dynamic_router_;
  static unordered_map<string, string> static_router_;
};

}  // namespace TimelineServer
