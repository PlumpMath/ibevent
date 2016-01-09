#ifndef IBEVENT_UTIL_H
#define IBEVENT_UTIL_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK_ZERO(val) if (val) { \
                          fprintf(stderr, "Checked failed at %s:%d, errno %d.\n", \
                                  __FILE__, __LINE__, errno); \
                          abort(); \
                        }

#define CHECK(val) CHECK_ZERO(!(val))

#endif
