#pragma once
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

class Epoll {
public:
  Epoll();
  ~Epoll();
  void epoll_add(SP_Channel req, int timeout);
  void epoll_mod(SP_Channel req, int timeout);
  void epoll_del(SP_Channel req);
  std::vector<std::shared_ptr<Channel>> poll();
  std::vector<std::shared_ptr<Channel>> getEventRequest(int events_num);
  void add_timer(std::shared_ptr<Channel> request_data, int timeout);
  int getEpollFd() const { return epollFd_; }
  void handleExpired();

private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_;
  std::shared_ptr<Channel> fd2chan_[MAXFDS];
  std::shared_ptr<HttpData> fd2http_[MAXFDS];
  TimerManager timerManager_;
};