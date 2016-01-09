// Linux 2.6.27+, glibc 2.9+

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "ibevent.h"
#include "internal.h"
#include "util.h"

#define MAX_EVENTS (16)

static int control_fd;
static struct ibevent control_event;

static void control_handler(struct ibevent *event) {
  uint64_t data;
  int ret;

  while ((ret = read(control_fd, &data, sizeof(data))) >= 0) {
    CHECK(ret == sizeof(data));
    // TODO(iceboy): read from ring buffer
  }
  CHECK(errno == EAGAIN);
}

static void *worker(void *id_ptr) {
  int id = (int)(intptr_t) id_ptr;
  struct epoll_event events[MAX_EVENTS];
  int nfds;
  int i;
  struct ibevent *ibevent;

  printf("[%d] Worker started.\n", id);
  while (1) {
    nfds = epoll_wait(ib_epoll_fd, events, MAX_EVENTS, -1);
    printf("[%d] Got %d events.\n", id, nfds);
    for (i = 0; i < nfds; ++i) {
      ibevent = (struct ibevent *)events[i].data.ptr;
      if ((events[i].events & EPOLLIN) && ibevent->in_handler) {
        ibevent->in_handler(ibevent);
      }
      if ((events[i].events & EPOLLOUT) && ibevent->out_handler) {
        ibevent->out_handler(ibevent);
      }
    }
    printf("[%d] Event handling completed.\n", id);
  }
}

void ibinit(void) {
  struct epoll_event event;
  ib_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  control_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  control_event.in_handler = control_handler;
  event.events = EPOLLIN | EPOLLET;
  event.data.ptr = &control_event;
  CHECK_ZERO(epoll_ctl(ib_epoll_fd, EPOLL_CTL_ADD, control_fd, &event))
}

// TODO(iceboy): Create worker threads according to CPU cores and app settings.
void ibmain(void) {
#define NUM_THREADS (2)
  pthread_t thread[NUM_THREADS];
  int i;
  for (i = 0; i < NUM_THREADS; ++i) {
    pthread_create(&thread[i], NULL, worker, (void *)(intptr_t)i);
  }
  for (i = 0; i < NUM_THREADS; ++i) {
    pthread_join(thread[i], NULL);
  }
}
