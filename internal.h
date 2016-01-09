#ifndef IBEVENT_INTERNAL_H
#define IBEVENT_INTERNAL_H

#include <ucontext.h>

extern int ib_epoll_fd;

struct ibevent {
  void (*in_handler)(struct ibevent *);
  void (*out_handler)(struct ibevent *);
};

struct overlapped {
  ucontext_t ucontext;
};

#endif
