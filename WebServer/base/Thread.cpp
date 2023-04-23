#include "Thread.h"
#include "CurrentThread.h"
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <linux/unistd.h>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "default";
}; // namespace CurrentThread

// ::syscall 是一个系统调用函数，可以调用底层的操作系统 API，
// 使用 SYS_gettid 参数获取当前线程的线程 ID
pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = gettid();
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
  }
}

struct ThreadData {
  typedef Thread::ThreadFunc ThreadFunc;
  // 线程函数指针
  ThreadFunc func_;
  // 线程名
  string name_;

  pid_t *tid_;
  // 计数器
  CountDownLatch *latch_;

  ThreadData(const ThreadFunc &func, const string &name, pid_t *tid,
             CountDownLatch *latch)
      : func_(func), name_(name), tid_(tid), latch_(latch) {}

  void runInThread() {
    *tid_ = CurrentThread::tid();
    tid_ = nullptr;
    latch_->countDown();
    latch_ = nullptr;

    CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
    // 设置线程名
    prctl(PR_SET_NAME, CurrentThread::t_threadName);
    func_();
    CurrentThread::t_threadName = "finished";
  }
};

void *startThread(void *obj) {
  ThreadData *data = static_cast<ThreadData *>(obj);
  data->runInThread();
  delete data;
  return nullptr;
}

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false), joined_(false), pthreadId_(0), tid_(0), func_(func),
      name_(n), latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  // 分离线程, 线程结束自动释放
  if (started_ && !joined_)
    pthread_detach(pthreadId_);
}

void Thread::setDefaultName() {
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;
  ThreadData *data = new ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthreadId_, nullptr, &startThread, data)) {
    // 线程创建失败，pthread_create返回非0值
    started_ = false;
    delete data;
  } else {
    latch_.wait();
    assert(tid_ > 0);
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, nullptr);
}