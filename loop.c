#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "ibevent.h"
#include "internal.h"
#include "util.h"

#define MAX_EVENTS (16)
#define CONTROL_RING_BITS (8)

int ib_epoll_fd;

struct control_ring {
  struct ring ring;
  void *data[1 << CONTROL_RING_BITS];
};

static struct control_ring control_ring;

static void control_handler(struct ring *ring, int num) {
  void *data;

  while (num--) {
    data = ring_read(ring);
    printf("%d\n", (int)(intptr_t)data);
  }
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

  assert(0);
}

void ibinit(void) {
  ib_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  ring_init(&control_ring.ring, control_handler, CONTROL_RING_BITS);
  for (int i = 0; i < 10; ++i) {
    ring_write(&control_ring.ring, (void *)(intptr_t)i);
  }
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
