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

    printf("valid: i: %i, v: %i\n", i, sum);
  }

  return max_i;
}

int cross_correlation_same(uint8_t *x, uint8_t *y, int n) {
  int max_v = INT_MIN;
  int max_i = 0;

  int m = n * 0.5;

  for (int i = -m; i <= m; ++i) {
    int sum = 0;

    for (int j = 0; j < n - abs(i); ++j) {
      sum += x[j + MAX(0, i)] * y[j + MAX(0, -i)];
    }

    if (sum > max_v) {
      max_v = sum;
      max_i = i + m;      
    }

    printf("same: i: %i, v: %i\n", i + m, sum);
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

    printf("full: i: %i, v: %i\n", i + m, sum);
  }

  return max_i;
}

  int calculateCrossCorrelation(uint8_t (*buffer)[2], int channel1, int channel2, int min_sample, int max_sample) {
    int correlation_length = (max_sample - min_sample) + 1;
    int max_correlation = INT_MIN;
    int max_correlation_idx = 0;

    int shift_count = correlation_length * 0.5;

    for (int i = -shift_count; i <= shift_count; ++i) {
        int sum = 0;

        for (int j = 0; j < correlation_length - abs(i); ++j) {
            int idx1 = min_sample + j + MAX(0, i);
            int idx2 = min_sample + j + MAX(0, -i);

            sum += buffer[idx1][channel1] * buffer[idx2][channel2];
        }

        if (sum > max_correlation) {
            max_correlation = sum;
            max_correlation_idx = i;
        }

        printf("same2: i: %i, v: %i\n", i + shift_count, sum);
    }

    return max_correlation_idx;
  }