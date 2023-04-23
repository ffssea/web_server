#include "Utils.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const int MAX_BUF = 4096;

// 从fd读取数据至buf
ssize_t readn(int fd, void *buf, size_t n) {
  ssize_t nleft = n;
  ssize_t nread = 0;
  ssize_t readSum = 0;
  char *ptr = (char *)buf;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR) {
        nread = 0;
      } else if (errno == EAGAIN) // 读完
      {
        return readSum;
      } else {
        return -1;
      }
    } else if (nread == 0) {
      break;
    }
    readSum += nread;
    nleft -= nread;
    ptr += nread;
  }
  return readSum;
}

ssize_t readn(int fd, std::string &inbuf, bool &zero) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (1) {
    char buf[MAX_BUF];
    if (nread = read(fd, buf, MAX_BUF) < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0) {
      zero = true;
      break;
    }

    readSum += nread;
    inbuf += std::string(buf, buf + nread);
  }
  return readSum;
}

ssize_t readn(int fd, std::string &inbuf) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (true) {
    char buf[MAX_BUF];
    nread = read(fd, buf, MAX_BUF);
    if (nread < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0)
      break;
    readSum += nread;
    inbuf += std::string(buf, buf + nread);
  }
  return readSum;
}

ssize_t writen(int fd, void *buf, ssize_t n) {
  size_t nleft = n;
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  char *ptr = (char *)buf;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN) {
          return writeSum;
        } else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  return writeSum;
}

ssize_t writen(int fd, std::string &sbuf) {
  size_t nleft = sbuf.size();
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  const char *ptr = sbuf.c_str();
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN)
          break;
        else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  if (writeSum == static_cast<int>(sbuf.size()))
    sbuf.clear();
  else
    sbuf = sbuf.substr(writeSum);
  return writeSum;
}

void handle_for_sigpipe() {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL))
    return;
}
int setSocketNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1)
    return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1)
    return -1;
  return 0;
}
/*
从而禁用 Nagle 算法，提高数据传输效率。

Nagle 算法是一种用于减少小分组发送的算法，它的实现是将小数据块收集到缓冲区中，
等到缓冲区填满后再一次性发送，这种算法虽然能减少网络传输的次数，
但是会引入一定的延迟，特别是对于实时性要求高的应用来说，这种延迟是无法接受的。

TCP_NODELAY 选项用于禁用 Nagle 算法，
使得每个数据包都可以及时发送，从而提高实时性。
具体实现就是通过 setsockopt() 函数将 TCP_NODELAY 选项设置为 1
*/
void setSocketNodelay(int fd) {
  int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof enable);
}
// shutDownWR 函数是关闭 TCP 连接的写半部分（half-close），
// 保留了读半部分（可以继续读取对方发送的数据）。
void shutDownWR(int fd) { shutdown(fd, SHUT_WR); }

int socket_bind_listen(int port) {
  if (port < 0 || port > 65535)
    return -1;

  int listen_fd = 0;
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1)
    return -1;
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) ==
      -1) {
    close(listen_fd);
    return -1;
  }
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof server_addr);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof server_addr) ==
      -1) {
    close(listen_fd);
    return -1;
  }

  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  if (listen_fd == -1) {
    close(listen_fd);
    return -1;
  }
  return listen_fd;
}