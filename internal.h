#ifndef IBEVENT_INTERNAL_H
#define IBEVENT_INTERNAL_H

#include <ucontext.h>

extern int ib_epoll_fd;

struct ibevent {
  void (*in_handler)(struct ibevent *);
  void (*out_handler)(struct ibevent *);
};

struct ring {
  struct ibevent ibevent;
  void (*handler)(struct ring *, int);
  int eventfd;
  volatile int readpos;
  volatile int writepos;
  int bits;
  void *volatile data[0];
};

extern void ring_init(struct ring *ring,
                      void (*handler)(struct ring *, int),
                      int bits);
extern void *ring_read(struct ring *ring);
extern void ring_write(struct ring *ring, void *data);

struct overlapped {
  ucontext_t ucontext;
};

#endif
