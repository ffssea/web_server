#pragma once
#include "Timer.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>

class EventLoop;
class TimerNode;
class Channel;

enum ProcessState {
  STATE_PARSE_URI = 1,
  STATE_PARSE_HEADERS,
  STATE_RECV_BODY,
  STATE_ANALYSIS,
  STATE_FINISH
};

enum URIState { PARSE_URI_AGAIN = 1, PARSE_URI_ERROR, PARSE_URI_SUCCESS };

enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ParseState {
  H_START = 0,
  H_KEY,
  H_COLON,
  H_SPACES_AFTER_COLON,
  H_VALUE,
  H_CR,
  H_LF,
  H_END_CR,
  H_END_LF
};

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

enum HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum HttpVersion { HTTP_10 = 1, HTTP_11 };

class MimeType {
private:
  static void init();
  static std::unordered_map<std::string, std::string> mime;
  MimeType();
  MimeType(const MimeType &m);

public:
  static std::string getMime(const std::string &suffix);

private:
  // pthread_once_t是一种数据类型，代表一个控制标志，
  // 用于保证一个函数仅仅被调用一次。
  // pthread_once函数配合这个标志来使用，
  // 确保在多线程环境下，一个函数只会被执行一次。
  static pthread_once_t once_control;
};

/**
enable_shared_from_this提供了一个成员函数shared_from_this()，
可以返回一个指向这个对象的shared_ptr，
如果当前对象没有以shared_ptr的方式构造或者已经销毁，
那么调用这个函数将会导致未定义的行为。*/
class HttpData : public std::enable_shared_from_this<HttpData> {
public:
  HttpData(EventLoop *loop, int connfd);
  ~HttpData() { close(fd_); }
  void reset();
  void seperateTimer();
  // 与此对象关联的Channel链接，
  // 以便事件循环能够跟踪计时器并适当地处理计时器事件
  void linkTimer(std::shared_ptr<TimerNode> mtimer) { timer_ = mtimer; }
  std::shared_ptr<Channel> getChannel() { return channel_; }
  EventLoop *getLoop() { return loop_; }
  void handleClose();
  void newEvent();

private:
  EventLoop *loop_;
  std::shared_ptr<Channel> channel_;
  int fd_;
  std::string inBuffer_;
  std::string outBuffer_;
  bool error_;
  ConnectionState connectionState_;

  HttpMethod method_;
  HttpVersion HTTPVersion_;
  std::string fileName_;
  std::string path_;
  int nowReadPos_;
  ProcessState state_;
  ParseState hState_;
  bool keepAlive_;
  std::map<std::string, std::string> headers_;
  std::weak_ptr<TimerNode> timer_;

  void handleRead();
  void handleWrite();
  void handleConn();
  void handleError(int fd, int err_num, std::string short_msg);
  URIState parseURI();
  HeaderState parseHeaders();
  AnalysisState analysisRequest();
};
