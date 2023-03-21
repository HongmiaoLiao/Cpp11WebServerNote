//
// Created by lhm on 2023/3/9.
//

#ifndef MODERNCPPWEBSERVER_HTTP_HTTPREQUEST_H_
#define MODERNCPPWEBSERVER_HTTP_HTTPREQUEST_H_

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
// #include <cerrno>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
 public:
  enum ParseState {
    kRequestLine,
    kHeaders,
    kBody,
    kFinish,
  };

  enum HttpCode {
    kNoRequest = 0,
    kGetRequest,
    kBadRequest,
    kNoResourse,
    kForbiddentRequest,
    kFileRequest,
    kInternalError,
    kClosedConnection,
  };

  // 调用Init()成员方法，解析内容置为空，解析状态置为解析请求行（从解析请求行开始）
  HttpRequest() {
    Init();
  }

  ~HttpRequest() = default;

  // 将解析的内容置为空，解析状态置为解析请求行（从解析请求行开始）
  void Init();
  // 解析HTTP请求
  HttpCode Parse(Buffer &buff);

  // 返回解析好的路径
  std::string Path() const;
  std::string &Path();
  // 返回解析好的请求方法
  std::string Method() const;
  // 返回请求好的版本
  std::string Version() const;
  // 在post请求体中根据给定的键获取对应的值，如果不存在则放回空字符串
  std::string GetPost(const std::string &key) const;
  std::string GetPost(const char *key) const;

  // 判断请求是否为长连接
  bool IsKeepAlive() const;

  ParseState State() const;

  /*
   * TODO
   * void HttpConn::ParseFormData();
   * void HttpConn::ParseJson();
   */

 private:
  // 解析HTTP请求的请求行，将对应信息写入对象成员变量
  bool ParseRequestLine_(const std::string &line);
  // 解析请求头的一行，即一个键值对信息（整个解析动作是一行一行读取的，请求头可能有多个键值对）
  void ParseHeader_(const std::string &line);
  // 解析请求体，Post请求有请求体，所以调用ParsePost_()来解析
  bool ParseBody_(const std::string &line);

  // 解析请求路径，将完整路径信息赋值给成员变量path_
  void ParsePath_();
  // 解析post请求，本项目中post请求只有登录请求，所以此方法作用仅为验证账号密码是否正确从而跳转到对应页面
  bool ParsePost_();

  // 解析post请求中的各组key = value
  void ParseFromUrlencoded_();

  // 验证输入的账户密码是否正确，即是否在数据库里面。根据is_login执行登录或注册
  static bool UserVerify(const std::string &name, const std::string &pwd, bool is_login);

  ParseState state_;  // 记录当前对http请求的解析状态
  std::string method_;  // 记录http请求的请求方法
  std::string path_;  // 记录http请求的资源路径
  std::string version_; // 记录http请求的当前版本（当前一般为1.1）
  std::string body_;  // 记录请求体内容
  std::unordered_map<std::string, std::string> header_; // 记录http请求的请求头键值对
  std::unordered_map<std::string, std::string> post_; // 记录post请求的请求体中的键值对

  static const std::unordered_set<std::string> kDefaultHtml;  // 可以请求的静态资源名称（不带后缀）
  static const std::unordered_map<std::string, int> kDefaultHtmlTag;  // 注册页面标识0， 登录页面标识1
  // 将16进制的字母转换为10进制数值，如果小于10直接返回，即A 10, B 11, C 12, D 13, E 14, F, 15
  static int ConverHex(char ch);
};

#endif //MODERNCPPWEBSERVER_HTTP_HTTPREQUEST_H_
