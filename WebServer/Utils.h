#pragma once
#include <stdio.h>
#include <string>

ssize_t readn(int fd, void *buf, size_t n);
ssize_t readn(int fd, std::string &inbuf, bool &zero);
ssize_t readn(int fd, std::string &inbuf);
ssize_t writen(int fd, void *buf, ssize_t n);
ssize_t writen(int fd, std::string &sbuf);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);
void shutDownWR(int fd);
int socket_bind_listen(int port);
