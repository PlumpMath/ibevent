#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "internal.h"
#include "util.h"

static void ring_handler(struct ibevent *ibevent) {
  struct ring *ring = (struct ring *)ibevent;
  uint64_t num;
  int ret = read(ring->eventfd, &num, sizeof(num));
  CHECK(ret == sizeof(num));
  ring->handler(ring, num >> (64 - ring->bits));
}

void ring_init(struct ring *ring,
               void (*handler)(struct ring *, int),
               int bits) {
  int ret;
  struct epoll_event event;

  ring->ibevent.in_handler = ring_handler;
  ring->ibevent.out_handler = NULL;
  ring->handler = handler;
  CHECK((ret = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) != -1);
  ring->eventfd = ret;
  ring->readpos = 0;
  ring->writepos = 0;
  ring->bits = bits;

  event.events = EPOLLIN | EPOLLET;
  event.data.ptr = ring;
  CHECK_ZERO(epoll_ctl(ib_epoll_fd, EPOLL_CTL_ADD, ring->eventfd, &event));
}

void *ring_read(struct ring *ring) {
  int mask = (1 << ring->bits) - 1;
  int pos = __sync_fetch_and_add(&ring->readpos, 1) & mask;
  return ring->data[pos];
}

void ring_write(struct ring *ring, void *data) {
  int mask = (1 << ring->bits) - 1;
  int pos;
  uint64_t num;

  pos = __sync_fetch_and_add(&ring->writepos, 1) & mask;
  ring->data[pos] = data;

  num = 1ULL << (64 - ring->bits);
  CHECK(write(ring->eventfd, &num, sizeof(num)) != -1);
}
