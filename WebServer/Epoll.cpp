#include "Epoll.h"
#include "Utils.h"
#include <arpa/inet.h>
#include <assert.h>
#include <deque>
#include <errno.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

typedef std::shared_ptr<Channel> SP_Channel;

Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
    assert(epollFd_ > 0);
}

Epoll::~Epoll() {}

void Epoll::epoll_add(SP_Channel req, int timeout) {
    int fd = req->getFd();
    if (timeout > 0) {
        add_timer(req, timeout);
        fd2http_[fd] = req->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = req->getEvents();
    req->EqualAndUpdateLastEvents();
    fd2chan_[fd] = req;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_add error");
        fd2chan_[fd].reset();
    }
}

void Epoll::epoll_mod(SP_Channel req, int timeout) {
    if (timeout > 0)
        add_timer(req, timeout);
    int fd = req->getFd();
    if (!req->EqualAndUpdateLastEvents()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = req->getEvents();
        if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            perror("epoll_mod fail");
        }
    }
}

void Epoll::epoll_del(SP_Channel req) {
    int fd = req->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = req->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("epoll_del error");
    }
    fd2chan_[fd].reset();
    fd2http_[fd].reset();
}

std::vector<SP_Channel> Epoll::poll() {
    while (true) {
        int event_count =
                epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
        if (event_count < 0)
            perror("epoll_wait error");
        std::vector<SP_Channel> req_data = getEventRequest(event_count);
        if (req_data.size() > 0)
            return req_data;
    }
}

void Epoll::handleExpired() { timerManager_.handleExpiredEvent(); }

std::vector<SP_Channel> Epoll::getEventRequest(int events_num) {
    std::vector<SP_Channel> req_data;
    for (int i = 0; i < events_num; ++i) {
        int fd = events_[i].data.fd;
        SP_Channel cur_req = fd2chan_[fd];
        if (cur_req) {
            cur_req->setRevents(events_[i].events);
            cur_req->setEvents(0);
            req_data.push_back(cur_req);
        } else {
            std::cout << "SP cur_req is invalid";
        }
    }
    return req_data;
}

void Epoll::add_timer(SP_Channel req_data, int timeout) {
    std::shared_ptr<HttpData> t = req_data->getHolder();
    if (t)
        timerManager_.addTimer(t, timeout);
    else
        std::cout << "timer add fail";
}