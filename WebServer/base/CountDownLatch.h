#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

// 线程同步机制
// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
class CountDownLatch : noncopyable {
public:
  explicit CountDownLatch(int count);
  void wait();
  void countDown();

private:
  // mutable : 如果CountDownLatch 为const, mutex_是可变的
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};