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

  HttpRequest() {
    Init();
  }

  ~HttpRequest() = default;

  void Init();
  bool parse(Buffer &buff);

  std::string Path() const;
  std::string &Path();
  std::string Method() const;
  std::string Version() const;
  std::string GetPost(const std::string &key) const;
  std::string GetPost(const char *key) const;

  bool IsKeepAlive() const;

  /*
   * TODO
   * void HttpConn::ParseFormData();
   * void HttpConn::ParseJson();
   */

 private:
  bool ParseRequestLine_(const std::string &line);
  void ParseHeader_(const std::string &line);
  void ParseBody_(const std::string &line);

  void ParsePath_();
  void ParsePost_();
  void ParseFromUrlencoded_();

  static bool UserVerify(const std::string &name, const std::string &pwd, bool is_login);

  ParseState state_;
  std::string method_;
  std::string path_;
  std::string version_;
  std::string body_;
  std::unordered_map<std::string, std::string> header_;
  std::unordered_map<std::string, std::string> post_;

  static const std::unordered_set<std::string> kDefaultHtml;
  static const std::unordered_map<std::string, int> kDefaultHtmlTag;
  static int ConverHex(char ch);
};

#endif //MODERNCPPWEBSERVER_HTTP_HTTPREQUEST_H_
