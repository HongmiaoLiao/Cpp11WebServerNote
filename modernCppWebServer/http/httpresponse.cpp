//
// Created by lhm on 2023/3/9.
//

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::kSuffixType = {
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

const std::unordered_map<int, std::string> HttpResponse::kCodeStatus = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::kCodePath = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse()
    : code_(-1),
      path_(""),
      src_dir_(""),
      is_keep_alive_(false),
      mm_file_(nullptr),
      mm_file_stat_({0}) {

}

HttpResponse::~HttpResponse() {
  UnmapFile();
}

// 执行响应对象的初始化
void HttpResponse::Init(const std::string &src_dir, std::string &path,
                        bool is_keep_alive, int code) {
  assert(src_dir != "");
  // assert(!src_dir.empty());
  if (mm_file_) {
    // 如果已经存在内存映射，则取消映射
    UnmapFile();
  }
  // 设置状态码、连接信息，路径信息，设置响应文件状态为空
  code_ = code;
  is_keep_alive_ = is_keep_alive;
  path_ = path;
  src_dir_ = src_dir;
  mm_file_ = nullptr;
  mm_file_stat_ = {0};
}

// 获取并拼接响应信息放入缓冲区
void HttpResponse::MakeResponse(Buffer &buff) {
  /* 原作者注释 判断请求的资源文件 */
  if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0
      || S_ISDIR(mm_file_stat_.st_mode)) {
    // 如果获取文件信息失败或者是一个目录，则code_设为404
    code_ = 404;
  } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
    // 如果不可读，code_设为403
    code_ = 403;
  } else if (code_ == -1) {
    // 获取文件成功且code_未赋值，则设置为200
    code_ = 200;
  }
  // 如果有错误，将路径指向错误页面
  ErrorHtml_();
  // 将状态行，响应头，响应体（出错时错误信息的响应体）放到缓冲区
  AddStateLine_(buff);
  AddHeader_(buff);
  AddContent_(buff);
}

// 返回响应文件映射到内存的起始地址指针，即返回mm_file_
char *HttpResponse::File() {
  return mm_file_;
}

// 返回响应文件的大小
size_t HttpResponse::FileLen() const {
  return mm_file_stat_.st_size;
}

// 根据错误的状态码信息（如果code_时错误码），将路径指向标识错误的html页面
void HttpResponse::ErrorHtml_() {
  if (kCodePath.count(code_) == 1) {
    // 如果错误码在错误码页面字典中，获取错误码页面，并获取错误码页面的信息
    path_ = kCodePath.find(code_)->second;
    // path_ = kCodePath[code_];  // 重载的运算符[]是非const的，不存在map中会添加默认值
    stat((src_dir_ + path_).data(), &mm_file_stat_);
  }
}

// 为响应消息添加状态行
void HttpResponse::AddStateLine_(Buffer &buff) {
  std::string status;
  // 如果当前状态码再状态码字典中，设置响应状态为对应的信息，否则统一设置为400
  if (kCodeStatus.count(code_) == 1) {
    status = kCodeStatus.find(code_)->second;
  } else {
    code_ = 400;
    status = kCodeStatus.find(400)->second;
  }
  // 将响应消息状态行放进缓冲区，以\r\n结尾
  buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

// 为响应消息添加消息报头，这里只添加是否为长连接的信息
void HttpResponse::AddHeader_(Buffer &buff) {
  // 本服务器响应消息报头只有是否长连接
  buff.Append("Connection: ");
  if (is_keep_alive_) {
    buff.Append("keep-alive\r\n");
    // 如果为长连接，超时时间为120m，最多接受6次请求就断开
    buff.Append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.Append("close\r\n");
  }
  //
  buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

// 添加响应体，如果出现错误就添加错误信息的响应体，否则只添加一个包含相应体长度信息的响应头字段
void HttpResponse::AddContent_(Buffer &buff) {
  int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
  if (src_fd < 0) {
    // 打开文件失败，则缓冲区写入错误信息的页面，直接返回
    ErrorContent(buff, "File NotFound!");
    return;
  }

  /* 原作者注释 */
  /* 将文件映射到内存提高文件的访问速度，MAP_PRIVATE 建立一个写入时拷贝的私有映射 */
  // 写入日志
  LOG_DEBUG("file path %s", (src_dir_ + path_).data());
  // 将响应文件的内容映射到当前进程内存虚拟地址0开始的空间
  int *mm_ret = (int *) mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
  // int *mm_ret = static_cast<int*> (mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
  if (*mm_ret == -1) {
    // 如果失败，mm_ret的值为-1，缓冲区写入错误信息的页面，直接返回
    ErrorContent(buff, "File NotFound");
    return;
  }
  // 如果映射成功，mm_ret指向的就是映射的地址，其内容为字符串（或者说以字符串方式读写），所以强转为字符串指针
  mm_file_ = (char *) mm_ret;
  // mm_file_ = static_cast<char*> (mm_ret);
  // 关闭文件描述符
  close(src_fd);
  // 为响应内容的长度添加响应头，有响应体的http的请求和响应才有这个内容，以\r\n\r\n结尾
  buff.Append("Content-length: " + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

// 删除响应文件在内存中的映射
void HttpResponse::UnmapFile() {
  if (mm_file_) {
    // 如果映射的指针不为空，则删除映射
    munmap(mm_file_, mm_file_stat_.st_size);
    mm_file_ = nullptr;
  }
}

// 获取响应的文件的类型
std::string HttpResponse::GetFileType_() {
  /* 原作者注释  判断文件类型 */
  // 获取文件的后缀名
  std::string::size_type idx = path_.find_last_of('.');
  if (idx == std::string::npos) {
    // 如果没有后缀名，则认为是text文件
    return "text/plain";
  }
  // 如果后缀名在文件类型字典中，则返回对应文件类型
  std::string suffix = path_.substr(idx);
  if (kSuffixType.count(suffix) == -1) {
    return kSuffixType.find(suffix)->second;
  }
  return "text/plain";
}

// 添加错误的响应体信息，将message信息写入到一个html页面中后放入缓冲区
// 注意这里message不能引用传递，因为直接传的字符串为const char*，不能转为string&，引用类型可能会改变原来的字符串
void HttpResponse::ErrorContent(Buffer &buff, std::string message) {
  std::string body;
  std::string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  // 如果状态码再字典中，则写入对应信息
  if (kCodeStatus.count(code_) == 1) {
    status = kCodeStatus.find(code_)->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";
  buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.Append(body);
}









