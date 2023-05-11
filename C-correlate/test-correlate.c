#include "correlate.h"

#define TAG "EXAMPLE"

void example() {
  uint8_t x[] = {1, 2, 3, 4, 5};
  uint8_t y[] = {4, 5, 6, 7, 8};
  int n = sizeof(x) / sizeof(x[0]);

  int valid = cross_correlation_valid(x, y, n);
  int same = cross_correlation_same(x, y, n);
  int full = cross_correlation_full(x, y, n);

  printf("Max shift (valid): %d", valid);
  printf("Max shift (same): %d", same);
  printf("Max shift (full): %d", full);
}

void main() {
  example();
}