#pragma once

#include "noncopyable.h"
#include <pthread.h>
#include <stdio.h>

// 封装互斥锁
class MutexLock : noncopyable {
public:
  MutexLock() { pthread_mutex_init(&mutex, NULL); }
  ~MutexLock() {
    // 析构时先加锁，避免互斥量被使用
    pthread_mutex_lock(&mutex);
    pthread_mutex_destroy(&mutex);
  }

  void lock() { pthread_mutex_lock(&mutex); }

  void unlock() { pthread_mutex_unlock(&mutex); }

  pthread_mutex_t *get() { return &mutex; }

private:
  pthread_mutex_t mutex;

  // 友元可以访问 mutex
  friend class Condition;
};

// 在构造函数中自动锁定互斥量，在析构函数中自动解锁互斥量，
// 确保互斥量在 MutexLockGuard 对象的生命周期内始终被持有和释放，
// 避免了忘记释放互斥量或异常导致互斥量未被释放的问题。
// 是一种常见的 RAII 设计模式的应用
class MutexLockGuard : noncopyable {
public:
  explicit MutexLockGuard(MutexLock &_mutex) : mutex(_mutex) {}
  ~MutexLockGuard() { mutex.unlock(); }

private:
  MutexLock &mutex;
};