#pragma once

// 设置拷贝构造函数和赋值操作为private，禁止其子类拷贝
class noncopyable {
protected:
  noncopyable() {}
  ~noncopyable() {}

private:
  noncopyable(const noncopyable &);
  const noncopyable &operator=(const noncopyable &);
};