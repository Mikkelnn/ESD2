#include "correlate.h"

#define TAG "EXAMPLE"

#define DATA_BUFFER 5 // 16000
#define N_CHANNELS 4

#define CHANNEL_IDX(sampel_idx, channel_idx) ((N_CHANNELS * sampel_idx) + (N_CHANNELS - 1 - channel_idx))

// #pragma pack(1)
// struct channels
// {
//   uint8_t ch0;
//   uint8_t ch1;
//   uint8_t ch2;
//   uint8_t ch3;
// };
// #pragma pack()

uint8_t u8_data[DATA_BUFFER][N_CHANNELS] = {0};

uint32_t data[DATA_BUFFER] = {0};

uint8_t * chData = (uint8_t *)&u8_data;

void arrTest_1() {
  // setup
  for (int i = 0; i<DATA_BUFFER;i++) {
    data[i] = (((3+i) << 24) | ((2+i) << 16) | ((1+i) << 8) | (0+i));
  }

  // print
  for (int i = 0; i < DATA_BUFFER; i++)
  {
    printf("idx: %i: %i, %i, %i, %i\n", i, chData[CHANNEL_IDX(i, 0)], chData[CHANNEL_IDX(i, 1)], chData[CHANNEL_IDX(i, 2)], chData[CHANNEL_IDX(i, 3)]);
  }
}

void arrTest_2() {
  // setup
  uint32_t * ptr = (void *)&u8_data;

  for (int i = 0; i < DATA_BUFFER; i++)
    (ptr[i]) = (((3+i) << 24) | ((2+i) << 16) | ((1+i) << 8) | (0+i));

  // print
  for (int i = 0; i < DATA_BUFFER; i++)
    printf("idx: %i: %i, %i, %i, %i\n", i, u8_data[i][0], u8_data[i][1], u8_data[i][2], u8_data[i][3]);
}

void arrTest_3() {
  // setup
  uint32_t * ptr = (uint32_t *)&u8_data;

  for (int i = 0; i < DATA_BUFFER; i++) {
    ptr[i] = (((3+i) << 24) | ((2+i) << 16) | ((1+i) << 8) | (0+i));
  }

  // print
  for (int i = 0; i < DATA_BUFFER; i++)
  {
    printf("idx: %i: %i, %i, %i, %i\n", i, chData[CHANNEL_IDX(i, 0)], chData[CHANNEL_IDX(i, 1)], chData[CHANNEL_IDX(i, 2)], chData[CHANNEL_IDX(i, 3)]);
  }
}

void example() {
  uint8_t x[] = {1, 2, 3, 4, 5};
  uint8_t y[] = {4, 5, 6, 7, 8};
  int n = 5; // sizeof(x); // / sizeof(x[0]);

  uint8_t buf[5][2] = {{1,4}, {2,5}, {3,6}, {4,7}, {5,8}};

  // int valid = cross_correlation_valid(x, y, n);
  int same = cross_correlation_same(x + 2, y + 2, n - 2);
  int same2 = calculateCrossCorrelation(buf, 0, 1, 2, 4);
  // int full = cross_correlation_full(x, y, n);

  // printf("Max shift (valid): %d\n", valid);
  printf("Max shift (same): %d\n", same);
  printf("Max shift (same2): %d\n", same2);

  // printf("Max shift (full): %d\n", full);
}

void main() {
  // example();
  // arrTest_2();
}