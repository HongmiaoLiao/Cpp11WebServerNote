//
// Created by lhm on 2023/3/11.
//

#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::kDefaultHtml{
    "/index", "/register", "/login", "/welcome", "/video", "/picture"
};

const std::unordered_map<std::string, int> HttpRequest::kDefaultHtmlTag{
    {"/register.html", 0}, {"/login.html", 1}
};

// 将解析的内容置为空，解析状态置为解析请求行（从解析请求行开始）
void HttpRequest::Init() {
  method_ = "";
  path_ = "";
  version_ = "";
  body_ = "";
  state_ = kRequestLine;
  header_.clear();
  post_.clear();
}

// 判断请求是否为长连接
bool HttpRequest::IsKeepAlive() const {
  if (header_.count("Connection") == 1) {
    // 如果请求头中有connection，判断http协议版本为1.1且请求Connection为keep-alive
    return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    // 函数IsKeepAlive是const，所以也不能用重载的[]
    // return header_["Connection"] == "keep-alive" && version_ == "1.1";
  }
  return false;
}

// 解析HTTP请求, 相比于原作者，修改了返回值
HttpRequest::HttpCode HttpRequest::Parse(Buffer &buff) {
  const char CRLF[] = "\r\n"; // 请求行和请求头每行结尾为CRLF
  if (buff.ReadableBytes() <= 0) {
    // 若没有内容可以被读取，直接返回false，这里是有问题的，应该返回一个NO_REQUEST状态，调用函数接收到该状态之后重新修改EPOLL事件注册一个EPOLLIN事件
    return kNoRequest;
    // return false
  }
  // 使用有限状态机方式解析http请求
  while (buff.ReadableBytes() && state_ != kFinish) {
    // 根据CRLF找到缓冲区中一行的结尾，然后取出这一行，line_end为'\r'位置，即\r\n的开头，也即一行有效字符的后一个字符
    const char *line_end = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
    // 根据查找到的位置，初始化一个字符串，即一行的有效字符内容。
    std::string line(buff.Peek(), line_end);
    switch (state_) {
      case kRequestLine:
        // 解析请求行，解析完成后state_变为kHeaders
        if (!ParseRequestLine_(line)) {
          return kBadRequest;
          // return false;
        }
        // 将请求路径放到成员变量中
        ParsePath_();
        break;
      case kHeaders:
        // 解析请求头，解析完成后state_变为kBody
        ParseHeader_(line);
        // if (buff.ReadableBytes() <= 2) {
        //   state_ = kFinish;
        // }
        // 如果是get请求，且目前解析完了请求行和请求头，当前状态为kBody
        if (state_ == kBody && method_ == "GET") {
          state_ = kFinish; // get请求，到此就可以结束，状态变为finish
          buff.RetrieveAll(); // 清空缓冲区
          return kGetRequest; // 返回状态为kGetRequest
        }
        break;
      case kBody:
        if (!ParseBody_(line)) {
          // 解析请求体失败
          return kNoRequest;
        }
        return kGetRequest;
      default:
        return kInternalError;
    }
    if (line_end == buff.BeginWrite()) {
      break;
    }
    buff.RetrieveUntil(line_end + 2);
  }
  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
  // 最后最好直接返回NO_REQUEST状态，表示如果执行到这一部分，说明请求没有接受完整，需要继续接受请求，若是请求完整的在之前就会return出while
  return kNoRequest;
}

// 解析请求路径，将完整路径信息赋值给成员变量path_
void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    // 如果路径为/ ，将路径置为主页，即index.html
    path_ = "/index.html";
  } else {
    for (auto &item : kDefaultHtml) {
      // 否则，从预定义的页面中查找，如果找到，则添加.html后缀后返回
      // 允许请求的都是静态变量，且是在程序中预先定义“写死”的，不写死或许可以初始化时读取路径然后写入字典
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

// 解析HTTP请求的请求行，将对应信息写入对象成员变量
bool HttpRequest::ParseRequestLine_(const std::string &line) {
  // 不包含空格的连续字符+空格+不包含空格的连续字符+空格+HTTP/不包含空格的连续字符，
  // ^[^ ]* [^ ]* HTTP/[^ ]*$ 是匹配整个串的正则，加上括号应该是为了给sub_match填充对应的值
  // std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    method_ = sub_match[1];
    path_ = sub_match[2];
    version_ = sub_match[3];
    state_ = kHeaders;  // 将解析状态置为kHeaders，接下来解析请求头
    return true;
  }
  // 解析失败则写入请求行错误日志，返回false
  LOG_ERROR("RequestLine Error");
  return false;
}

// 解析请求头的一行，即一个键值对信息（整个解析动作是一行一行读取的，请求头可能有多个键值对）
void HttpRequest::ParseHeader_(const std::string &line) {
  // 请求头一行为keystr: valstr
  // 正则表达式为：非:的连续字符 + : + 0个或1个空格 + 除换行符之外的连续字符
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch sub_match;
  if (std::regex_match(line, sub_match, patten)) {
    // 匹配成功，将键值对放入请求头的hash表中
    header_[sub_match[1]] = sub_match[2];
  } else {
    // 匹配失败，此时的行应该为回车符+换行符，http协议中用于分割请求头和请求体，状态置为kBody接下来解析请求体
    state_ = kBody;
  }
}

// 解析请求体，Post请求有请求体，所以调用ParsePost_()来解析
bool HttpRequest::ParseBody_(const std::string &line) {
  body_ = line; // 将请求体内容赋值给成员变量
  if (!ParsePost_()) {
    return false;
  }; // 解析post请求（如果是post请求的话）
  state_ = kFinish; // 将解析状态置为解析完成
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());  // 日志记下解析信息
  return true;
}

// 将16进制的字母转换为10进制数值，如果小于10直接返回，即A 10, B 11, C 12, D 13, E 14, F, 15
int HttpRequest::ConverHex(char ch) {
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return ch;
}

// 解析post请求，本项目中post请求只有登录请求，所以此方法作用仅为验证账号密码是否正确从而跳转到对应页面
bool HttpRequest::ParsePost_() {
  if (body_.size() < atol(header_["Content-Length"].c_str())) {
    // 判断post数据是否接收完整，不完整返回false
    return false;
  }
  if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
    // 如果是POST，"application/x-www-form-urlencoded"表示将post将表单内的数据转换为Key-Value。
    ParseFromUrlencoded_();
    if (kDefaultHtmlTag.count(path_)) {
      int tag = kDefaultHtmlTag.find(path_)->second;
      LOG_DEBUG("Tag:%d", tag);
      if (tag == 0 || tag == 1) {
        bool is_login = (tag == 1);
        // 验证账户密码是否正确
        if (UserVerify(post_["username"], post_["password"], is_login)) {
          path_ = "/welcome.html";
        } else {
          path_ = "/error.html";
        }
      }
    }
  }
  return true;
}

// 解析post请求中的各组key = value
void HttpRequest::ParseFromUrlencoded_() {
  // username=admin&password=123456
  if (body_.size() == 0) {
    // 请求体为空，直接返回
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
        // 等号之前是key
        key = body_.substr(j, i - j);
        j = i + 1;
        break;
      case '+':
        // + 改为空格，因为浏览器会将空格编码为+号
        body_[i] = ' ';
        break;
      case '%':
        // 浏览器会将非字母字符编码为%16进制，将其复原
        num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
        body_[i + 2] = num % 10 + '0';
        body_[i + 1] = num / 10 + '0';
        i += 2;
        break;
      case '&':
        // 解析完一组键值，也即&前是value
        value = body_.substr(j, i - j);
        j = i + 1;
        // 将解析好的键值放到post请求字典中
        post_[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        break;
      default:
        break;
    }
  }
  assert(j <= i);
  if (post_.count(key) == 0 && j < i) {
    // 将最后一对键值放到post请求字典中
    value = body_.substr(j, i - j);
    post_[key] = value;
  }
}

// 验证输入的账户密码是否正确，即是否在数据库里面。根据is_login执行登录或注册
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool is_login) {
  if (name == "" || pwd == "") {
    // 如果用户名和密码为空，直接返回false
    return false;
  }
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  // 从数据库连接池中取出一个连接，连接池初始化时已经指定了数据库名
  MYSQL *sql;
  // 使用RAII机制, 这里原作者没用临时变量，构造后立马就析构了，这样连接池内存在和当前重复的连接
  // SqlConnRAII (&sql, SqlConnPool::Instance());
  // 我加了个tmp临时变量，令其在作用域结束，也就是这个函数结束时再析构。
  SqlConnRAII tmp(&sql, SqlConnPool::Instance());
  assert(sql);

  // 初始化mysql的一些接收信息的结构体指针
  bool flag = false;  // 标识是否是注册
  unsigned int j = 0;
  char order[256] = {0};
  MYSQL_FIELD *fields = nullptr;
  MYSQL_RES *res = nullptr;

  if (!is_login) {
    // 如果为注册，flag设为true
    flag = true;
  }
  /* 原作者注释， 查询用户及密码 */
  snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());
  LOG_DEBUG("%s", order);

  // 执行查询
  if (mysql_query(sql, order)) {
    // 如果查询失败（成功为0，识别非0），释放结果集
    mysql_free_result(res);
    return false;
  }
  //
  res = mysql_store_result(sql);
  j = mysql_num_fields(res);
  fields = mysql_fetch_fields(res);

  // 从查询结果中一行行取出，然后判断账号和密码是否正确（理论上应该只查询出一行）
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
  // 释放结果集，防止内存泄漏
  mysql_free_result(res);

  /* 原作者注释 注册行为 且 用户名未被使用 */
  if (!is_login && flag == true) {
    LOG_DEBUG("register!");
    bzero(order, 256);
    snprintf(order, 256, "INSERT INTO user(username, passwd) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
      LOG_DEBUG("Insert error!");
      flag = false;
    }
    flag = true;
  }
  // SqlConnPool::Instance()->FreeConn(sql);  // 作用域结束自动释放不需要在这释放
  LOG_DEBUG("UserVerify success!!");
  return flag;
}

// 返回解析好的路径
std::string HttpRequest::Path() const {
  return path_;
}

// 返回解析好的路径
std::string &HttpRequest::Path() {
  return path_;
}

// 返回解析好的请求方法
std::string HttpRequest::Method() const {
  return method_;
}

// 返回请求好的版本
std::string HttpRequest::Version() const {
  return version_;
}

// 在post请求体中根据给定的键获取对应的值，如果不存在则放回空字符串
std::string HttpRequest::GetPost(const std::string &key) const {
  assert(key != "");
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}

// 上个函数的const char* 字符串重载版本
std::string HttpRequest::GetPost(const char *key) const {
  assert(key != nullptr);
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}

HttpRequest::ParseState HttpRequest::State() const {
  return state_;
}



















