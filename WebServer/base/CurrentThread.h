#pragma once
#include <stdint.h>

// 保存和线程有关的信息和函数
namespace CurrentThread {
// 线程局部存储 __thread
// extern可以置于变量或者函数前，以标示变量或者函数的定义在别的文件中
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

void cacheTid();

inline int tid() {
  // __builtin_expect 做了一个条件分支的预测。
  // 告诉编译器条件表达式的期望结果，让编译器在生成代码时优化预测正确的分支。
  // 在这个条件表达式中，预测的结果是 t_cachedTid == 0 的概率较小，
  // 因此将其期望值设为 0，表示更可能是条件表达式的另一侧为真，
  // 即 t_cachedTid != 0。
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

inline const char *tidString() { return t_tidString; }

inline int tidStringLength() { return t_tidStringLength; }
inline const char *name() { return t_threadName; }
}; // namespace CurrentThread
