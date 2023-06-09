#include "EventLoop.h"
#include "Utils.h"
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>

using namespace std;

// 线程局部变量
__thread EventLoop *t_loopInThisThread = 0;

int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    cout << "failed in eventfd\n";
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), poller_(new Epoll()), wakeupFd_(createEventfd()),
      quit_(false), eventHandling_(false), callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      pwakeupChannel_(new Channel(this, wakeupFd_)) {
  if (t_loopInThisThread) {
    cout << "Another EventLoop " << t_loopInThisThread
         << " exists in this thread " << threadId_;
  } else {
    t_loopInThisThread = this;
  }
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
  pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
  poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn() { updatePoller(pwakeupChannel_, 0); }

EventLoop::~EventLoop() {
  close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char *)(&one), sizeof one);
  if (n != sizeof one) {
    cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8\n";
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    cout << "EventLoop::handleRead() writes " << n << " bytes instead of 8\n";
  }
}

void EventLoop::runInLoop(Functor &&cb) {
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor &&cb) {
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }
  if (!isInLoopThread() || callingPendingFunctors_)
    wakeup();
}

void EventLoop::loop() {
  assert(!looping_);
  assert(isInLoopThread());
  looping_ = true;
  quit_ = false;
  std::vector<SP_Channel> ret;
  while (!quit_) {
    ret.clear();
    ret = poller_->poll();
    eventHandling_ = true;
    for (auto &it : ret)
      it->handleEvents();
    eventHandling_ = false;
    doPendingFunctors();
    poller_->handleExpired();
  }
  looping_ = false;
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }
  for (size_t i = 0; i < functors.size(); ++i)
    functors[i]();
  callingPendingFunctors_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!isInLoopThread())
    wakeup();
}