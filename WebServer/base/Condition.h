#pragma once

#include "MutexLock.h"
#include "noncopyable.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

// 竞态条件类
class Condition {

public:
  // 禁止隐式类型转换
  explicit Condition(MutexLock &_mutex) : mutex(_mutex) {
    pthread_cond_init(&cond, nullptr);
  }
  ~Condition() { pthread_cond_destroy(&cond); }
  // 等待条件
  void wait() { pthread_cond_wait(&cond, mutex.get()); }
  // 唤醒一个等待线程
  void notify() { pthread_cond_signal(&cond); }
  // 唤醒所有等待线程
  void notifyAll() { pthread_cond_broadcast(&cond); }

  /**
      pthread_cond_timedwait(&cond, mutex.get())
      等待条件变量 cond，同时传入一个互斥锁 mutex。
      这个函数会将当前线程阻塞，直到满足以下任意一种情况：

      - 条件变量 cond 被另一个线程唤醒；
      - 截止时间到达，函数返回 ETIMEDOUT。
  **/
  bool waitForSeconds(int seconds) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += static_cast<time_t>(seconds);
    // 如果函数返回值为 ETIMEDOUT，说明等待超时，返回 true；否则返回 false。
    return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
  }

private:
  MutexLock &mutex;
  pthread_cond_t cond;
};