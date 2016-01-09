#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "ibevent.h"
#include "internal.h"
#include "util.h"

#define MAX_EVENTS (16)
#define GO_RING_BITS (8)
#define GO_STACK_SIZE (16384)

int ib_epoll_fd;

struct go_ring {
  struct ring ring;
  void *data[1 << GO_RING_BITS];
};

static struct go_ring go_ring;

struct go_stack {
  union {
    uint8_t stack[GO_STACK_SIZE];
    void (*func)();
  };
};

static void go_handler(struct ring *ring, int num) {
  struct go_stack *stack;
  ucontext_t uc, ouc;
  getcontext(&uc);

  while (num--) {
    stack = ring_read(ring);
    // TODO(iceboy): serious scalability problem here...
    uc.uc_stack.ss_sp = stack->stack;
    uc.uc_stack.ss_flags = 0;
    uc.uc_stack.ss_size = sizeof(struct go_stack);
    uc.uc_link = &ouc;
    makecontext(&uc, stack->func, 0);
    swapcontext(&ouc, &uc);
    free(stack);
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
  ring_init(&go_ring.ring, go_handler, GO_RING_BITS);
}

// TODO(iceboy): Allow user to pass in context pointers.
void ibgo(void (*func)(void)) {
  // TODO(iceboy): Optimize allocator.
  struct go_stack *stack = malloc(sizeof(struct go_stack));
  stack->func = func;
  ring_write(&go_ring.ring, stack);
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
