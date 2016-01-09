#include <stdio.h>

#include "ibevent.h"

void foo(void) {
  printf("It works!\n");
}

int main(void) {
  ibinit();
  ibgo(foo);
  ibmain();
}
