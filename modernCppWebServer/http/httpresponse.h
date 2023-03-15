//
// Created by lhm on 2023/3/9.
//

#ifndef MODERNCPPWEBSERVER_HTTP_HTTPRESPONSE_H_
#define MODERNCPPWEBSERVER_HTTP_HTTPRESPONSE_H_

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  // 执行响应对象的初始化
  void Init(const std::string &src_dir, std::string &path,
            bool is_keep_alive = false, int code = -1);
  // 获取并拼接响应信息放入缓冲区
  void MakeResponse(Buffer &buff);
  // 删除响应文件在内存中的映射
  void UnmapFile();
  // 返回响应文件映射到内存的起始地址指针，即返回mm_file_
  char *File();
  // 返回响应文件的大小
  size_t FileLen() const;
  // 添加错误的响应体信息，将message信息写入到一个html页面中后放入缓冲区
  void ErrorContent(Buffer &buff, std::string message);
  int Code() const {
    return code_;
  }

 private:
  // 为响应消息添加状态行
  void AddStateLine_(Buffer &buff);
  // 为响应消息添加消息报头
  void AddHeader_(Buffer &buff);
  // 添加响应体，如果出现错误就添加错误信息的响应题，否则只添加一个包含相应体长度信息的响应头字段
  void AddContent_(Buffer &buff);

  // 根据错误的状态码信息（如果code_时错误码），将路径指向标识错误的html页面
  void ErrorHtml_();
  // 获取响应的文件的类型
  std::string GetFileType_();

  int code_;  // 当前响应的状态码
  bool is_keep_alive_;  // 标识是否为长连接
  std::string path_;  // 工作目录下的资源路径的字符串
  std::string src_dir_; // 工作目录路径的字符串

  char *mm_file_; // 响应文件内容映射到内存的指针
  struct stat mm_file_stat_;  // 响应读取的文件的信息

  // 静态常量的字典，标识文件的[MIME类型]
  static const std::unordered_map<std::string, std::string> kSuffixType;
  // 静态常量的字典，标识状态码对应的状态信息
  static const std::unordered_map<int, std::string> kCodeStatus;
  // 静态常量的字典，标识错误码对应的错误页面的工作路径
  static const std::unordered_map<int, std::string> kCodePath;
};

#endif //MODERNCPPWEBSERVER_HTTP_HTTPRESPONSE_H_
