#pragma once
#include "CountDownLatch.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

// 封装线程操作
class Thread : noncopyable {
public:
  // 不接受参数和返回值的线程函数对象，用于启动新线程时的回调函数类型
  typedef std::function<void()> ThreadFunc;
  explicit Thread(const ThreadFunc &, const std::string &name = std::string());
  ~Thread();
  void start();
  // 等待结束
  int join();
  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; }

private:
  void setDefaultName();

  bool started_;
  bool joined_;
  pthread_t pthreadId_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  CountDownLatch latch_;
};