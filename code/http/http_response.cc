#include "http_response.h"

namespace TimelineServer {

// 预设文件类型
const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

// 预设状态码
const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

// 预设错误代码页面文件
const unordered_map<int, string> HttpResponse::ERROR_CODE_FILE = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
  // 响应状态
  code_ = -1;
  is_keep_alive_ = false;

  // 资源文件路径
  src_dir_ = "";
  file_path_ = "";

  // 返回的资源文件(映射到内存的指针)
  mm_file_ = nullptr;
  mm_file_stat_ = {0};
}

HttpResponse::~HttpResponse() {
  // 回收内存中的资源文件
  unmap_file();
}

void HttpResponse::init(const std::string& src_dir, std::string& file_path,
                        bool is_keep_alive, int code) {
  src_dir_ = src_dir;
  file_path_ = file_path;
  is_keep_alive_ = is_keep_alive;
  code_ = code;
}

void HttpResponse::make_response(Buffer& buffer) {
  // 判断响应码
  if (stat((src_dir_ + file_path_).data(), &mm_file_stat_) < 0 ||
      S_ISDIR(mm_file_stat_.st_mode)) {
    // https://man7.org/linux/man-pages/man2/lstat.2.html
    // stat 失败时返回 -1
    // 或者找到的目标是文件夹
    code_ = 404;
  } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
    // IROTH ==> Is Readable for OTHers
    // Read permission bit for other users. Usually 04.
    // 不允许读取
    code_ = 403;
  } else if (code_ == -1) {
    // 可能初始化的时候传入了其他错误码,不应该覆盖成200
    code_ = 200;
  }

  // 响应状态码
  response_to_code_();
  // 填充内容
  add_state_line_(buffer);
  add_header_(buffer);
  add_content_(buffer);
}

char* HttpResponse::file() { return mm_file_; }

void HttpResponse::unmap_file() {
  if (mm_file_) {
    munmap(mm_file_, mm_file_stat_.st_size);
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
  }
}

void HttpResponse::add_state_line_(Buffer& buff) {
  string status;

  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    code_ = 400;
    status = CODE_STATUS.find(code_)->second;
  }

  buff.write_buffer("HTTP/1.1" + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::add_header_(Buffer& buff) {
  buff.write_buffer("Connection: ");
  if (is_keep_alive_) {
    buff.write_buffer("keep-alive\r\n");
    buff.write_buffer("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.write_buffer("close\r\n");
  }

  buff.write_buffer("Content-type: " + get_file_type_() + "\r\n");
}

void HttpResponse::add_content_(Buffer& buff) {
  int fd = open((src_dir_ + file_path_).data(), O_RDONLY);
  if (fd < 0) {
    add_error_content(buff, "File Not Found.");
    LOG_DEBUG("File \"%s\" not found.", (src_dir_ + file_path_).data());
    return;
  }

  int* mm_file =
      (int*)mmap(nullptr, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (*mm_file == -1) {
    add_error_content(buff, "File Not Found.");
    LOG_DEBUG("File \"%s\" not found.", (src_dir_ + file_path_).data());
    return;
  }

  mm_file_ = (char*)mm_file;
  close(fd);
  buff.write_buffer("Content-length: " + std::to_string(mm_file_stat_.st_size) +
                    "\r\n\r\n");
}

void HttpResponse::add_error_content(Buffer& buff, std::string message) {
  string body;
  string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";

  buff.write_buffer("Content-length: " + std::to_string(body.size()) +
                    "\r\n\r\n");
  buff.write_buffer(body);
}

void HttpResponse::response_to_code_() {
  if (ERROR_CODE_FILE.count(code_) == 1) {
    file_path_ = ERROR_CODE_FILE.find(code_)->second;
    stat((src_dir_ + file_path_).data(), &mm_file_stat_);
  }
}

string HttpResponse::get_file_type_() {
  string::size_type idx = file_path_.find_last_of('.');

  // 无后缀
  if (idx == string::npos) {
    return "text/plain";
  }

  string suffix = file_path_.substr(idx);
  // 后缀在预设内
  if (SUFFIX_TYPE.count(suffix) == 1) {
    return SUFFIX_TYPE.find(suffix)->second;
  }

  // 后缀不在预设内
  return "text/plain";
}

}  // namespace TimelineServer
