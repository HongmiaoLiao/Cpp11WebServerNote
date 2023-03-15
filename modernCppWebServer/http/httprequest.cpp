//
// Created by lhm on 2023/3/11.
//

#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::kDefaultHtml{
    "/index", "/register", "login", "/welcome", "/video", "/picture"
};

const std::unordered_map<std::string, int> HttpRequest::kDefaultHtmlTag{
    {"/register.html", 0}, {"/login.html", 1}
};

void HttpRequest::Init() {
  method_ = "";
  path_ = "";
  version_ = "";
  body_ = "";
  state_ = kRequestLine;
  header_.clear();
  post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
  if (header_.count("Connection") == 1) {
    return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    // 函数IsKeepAlive是const，所以也不能用重载的[]
    // return header_["Connection"] == "keep-alive" && version_ == "1.1";
  }
  return false;
}

bool HttpRequest::parse(Buffer &buff) {
  const char CRLF[] = "\r\n";
  if (buff.ReadableBytes() <= 0) {
    return false;
  }
  while (buff.ReadableBytes() && state_ != kFinish) {
    const char *line_end = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
    std::string line(buff.Peek(), line_end);
    switch (state_) {
      case kRequestLine:
        if (!ParseRequestLine_(line)) {
          return false;
        }
        ParsePath_();
        break;
      case kHeaders:ParseHeader_(line);
        if (buff.ReadableBytes() <= 2) {
          state_ = kFinish;
        }
        break;
      case kBody:ParseBody_(line);
        break;
    }
    if (line_end == buff.BeginWrite()) {
      break;
    }
    buff.RetrieveUntil(line_end + 2);
  }
  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
  return true;
}

void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (auto &item : kDefaultHtml) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

bool HttpRequest::ParseRequestLine_(const std::string &line) {
  // 不包含空格的连续字符+空格+不包含空格的连续字符+空格+HTTP/不包含空格的连续字符，
  // ^[^ ]* [^ ]* HTTP/[^ ]*$ 是匹配整个串的正则，加上括号应该是为了给sub_match填充对应的值
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    method_ = sub_match[1];
    path_ = sub_match[2];
    version_ = sub_match[3];
    state_ = kHeaders;
    return true;
  }
  LOG_ERROR("RequestLine Error");
  return false;
}

void HttpRequest::ParseHeader_(const std::string &line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    header_[sub_match[1]] = sub_match[2];
  } else {
    state_ = kBody;
  }
}

void HttpRequest::ParseBody_(const std::string &line) {
  body_ = line;
  ParsePost_();
  state_ = kFinish;
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return ch;
}

void HttpRequest::ParsePost_() {
  if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParseFromUrlencoded_();
    if (kDefaultHtmlTag.count(path_)) {
      int tag = kDefaultHtmlTag.find(path_)->second;
      LOG_DEBUG("Tag:%d", tag);
      if (tag == 0 || tag == 1) {
        bool is_login = (tag == 1);
        if (UserVerify(post_["username"], post_["password"], is_login)) {
          path_ = "/welcome.html";
        } else {
          path_ = "/error.html";
        }
      }
    }
  }
}

void HttpRequest::ParseFromUrlencoded_() {
  if (body_.size() == 0) {
    return;
  }
  // if (body_.empty()) {
  //   return ;
  // }
  std::string key;
  std::string value;
  int num = 0;
  int n = body_.size();
  int i = 0;
  int j = 0;

  for (; i < n; ++i) {
    char ch = body_[i];
    switch (ch) {
      case '=':
        key = body_.substr(j, i - j);
        j = i + 1;
        break;
      case '+':
        body_[i] = ' ';
        break;
      case '%':
        num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
        body_[i + 2] = num % 10 + '0';
        body_[i + 1] = num / 10 + '0';
        i += 2;
        break;
      case '&':
        value = body_.substr(j, i - j);
        j = i + 1;
        post_[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        break;
      default:
        break;
    }
  }
  assert(j <= i);
  if (post_.count(key) == 0 && j < i) {
    value = body_.substr(j, i - j);
    post_[key] = value;
  }
}
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool is_login) {
  if (name == "" || pwd == "") {
    return false;
  }
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  MYSQL *sql;
  SqlConnRAII(&sql, SqlConnPool::Instance());
  assert(sql);

  bool flag = false;
  unsigned int j = 0;
  char order[256] = {0};
  MYSQL_FIELD *fields = nullptr;
  MYSQL_RES *res = nullptr;

  if (!is_login) {
    flag = true;
  }
  /* 原作者注释， 查询用户及密码 */
  snprintf(order, 256, "SELECT name, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
  LOG_DEBUG("%s", order);

  if (mysql_query(sql, order)) {
    mysql_free_result(res);
  }
  res = mysql_store_result(sql);
  j = mysql_num_fields(res);
  fields = mysql_fetch_fields(res);

  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    std::string password(row[1]);
    /* 原作者注释 注册行为 且 用户名未被使用 */
    if (is_login) {
      if (pwd == password) {
        flag = true;
      } else {
        flag = false;
        LOG_DEBUG("pwd error");
      }
    } else {
      flag = false;
      LOG_DEBUG("user used!");
    }
  }
  mysql_free_result(res);

  /* 原作者注释 注册行为 且 用户名未被使用 */
  if (!is_login && flag == true) {
    LOG_DEBUG("register!");
    bzero(order, 256);
    snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
      LOG_DEBUG("Insert error!");
      flag = false;
    }
    flag = true;
  }
  SqlConnPool::Instance()->FreeConn(sql);
  LOG_DEBUG("UserVerify success!!");
  return flag;
}

std::string HttpRequest::Path() const {
  return path_;
}

std::string &HttpRequest::Path() {
  return path_;
}

std::string HttpRequest::Method() const {
  return method_;
}

std::string HttpRequest::Version() const {
  return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const {
  assert(key != "");
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}

std::string HttpRequest::GetPost(const char *key) const {
  assert(key != nullptr);
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}



















