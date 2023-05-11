#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

int cross_correlation_valid(uint8_t *x, uint8_t *y, int n) {
  int max_v = INT_MIN;
  int max_i = 0;

  for (int i = 0; i < n; ++i) {
    int sum = 0;

    for (int j = 0; j < n - i; ++j) {
      sum += x[j] * y[j + i];
    }

    if (sum > max_v) {
      max_v = sum;
      max_i = i;
    }
  }

  return max_i;
}

int cross_correlation_same(uint8_t *x, uint8_t *y, int n) {
  int max_v = INT_MIN;
  int max_i = 0;

  int m = n / 2;

  for (int i = -m; i <= m; ++i) {
    int sum = 0;

    for (int j = 0; j < n - abs(i); ++j) {
      sum += x[j + MAX(0, i)] * y[j + MAX(0, -i)];
    }

    if (sum > max_v) {
      max_v = sum;
      max_i = i + m;
    }
  }

  return max_i;
}

int cross_correlation_full(uint8_t *x, uint8_t *y, int n) {
  int max_v = INT_MIN;
  int max_i = 0;

  int m = n - 1;

  for (int i = -m; i <= m; ++i) {
    int sum = 0;

    for (int j = 0; j < n - abs(i); ++j) {
      sum += x[j + MAX(0, i)] * y[j + MAX(0, -i)];
    }

    if (sum > max_v) {
      max_v = sum;
      max_i = i + m;
    }
  }

  return max_i;
}
