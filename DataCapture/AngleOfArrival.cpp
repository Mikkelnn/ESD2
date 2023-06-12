#include <math.h>
#include <limits.h>
#include "filter-c/filter.h"

#define DATA_BUFFER 16000
#define N_CHANNELS 8
#define SOUND_SPEED_CM_PER_S 34300
#define MIC_DIST_CM 7.5
#define PULSE_WIDTH_MS 1

#define SAMPELING_RATE 80000  // Assuming unit sampling rate, modify if needed
#define SAMPLE_PERIOD_US ((1.0 / SAMPELING_RATE) * 1000000/* 10^6 */)
#define SAMPLES_PER_MS (SAMPELING_RATE * 0.001)
#define PULSE_WIDTH_SAMPLES (PULSE_WIDTH_MS * SAMPLES_PER_MS)

// settings for digital filter
#define BAND_PASS_LOW_CUT 38200
#define BAND_PASS_HIGH_CUT 38700
#define BAND_PASS_ORDER 5


class AngleOfArrival {
public:
  AngleOfArrival() {
    calculateAngleLookupTable();
  }

  int calculateAngleAndDistance(uint8_t (*buffer)[N_CHANNELS], int& angle, int& distance_cm) {
    int used_channels[N_CHANNELS];
    int num_used_channels = findUsedChannels(buffer, used_channels);

    if (num_used_channels < 2) return 1;

    // filter
    BWBandPass* filter = create_bw_band_pass_filter(BAND_PASS_ORDER, SAMPELING_RATE, BAND_PASS_LOW_CUT, BAND_PASS_HIGH_CUT);

    

    // process
    int signalDelay = calculateSignalDelay(buffer, used_channels[0], used_channels[1]);
    int max_correlation_sample = calculateMaxCorrelationSample(signalDelay);
    int min_correlation_sample = calculateMinCorrelationSample(signalDelay);
    int max_correlation_idx = calculateCrossCorrelation(buffer, used_channels[0], used_channels[1], min_correlation_sample, max_correlation_sample);
    int relative_shift = calculateRelativeShift(max_correlation_idx, min_correlation_sample, max_correlation_sample);

    distance_cm = calculateDistance(signalDelay);
    angle = calculateAngleFromShift(relative_shift);

    return 0;
  }

private:
  int angleLookup[181];  // -90 to +90 degrees
  uint8_t signal_block[PULSE_WIDTH_SAMPLES] = {255};

  void calculateAngleLookupTable() {
    for (int i = -90; i <= 90; i++) {
      double distance = MIC_DIST_CM * sin(deg2rad(i));
      double delay_us = (distance / SOUND_SPEED_CM_PER_S) * 1000000.0;
      angleLookup[i + 90] = (int)delay_us;
    }
  }

  int findUsedChannels(uint8_t (*buffer)[N_CHANNELS], int* used_channels) {
    int num_used_channels = 0;
    for (int i = 0; i < N_CHANNELS; i++) {
        for (int j = 0; j < DATA_BUFFER; j++) {
            if (buffer[j][i] > 0) {
              used_channels[num_used_channels] = i;
              num_used_channels++;
              continue;
        }
      } 
    }
    return num_used_channels;
  }

  int calculateSignalDelay(uint8_t (*buffer)[N_CHANNELS], int channel1, int channel2) {
    int blockShift_0 = 0;
    int blockShift_1 = 0;
    int max_correlation_0 = 0;
    int max_correlation_1 = 0;

    for (int i = 0; i < DATA_BUFFER - PULSE_WIDTH_SAMPLES; i++) {
      int correlation_0 = 0;
      int correlation_1 = 0;            
      for (int j = 0; j < PULSE_WIDTH_SAMPLES; j++) {
        correlation_0 += buffer[i + j][channel1] * signal_block[j];
        correlation_1 += buffer[i + j][channel2] * signal_block[j];
      }
      if (correlation_0 > max_correlation_0) {
        max_correlation_0 = correlation_0;
        blockShift_0 = i;
      }
      if (correlation_1 > max_correlation_1) {
        correlation_1 = max_correlation_1;
        blockShift_1 = i;
      }
    }

    int signalDelay = (blockShift_0 < blockShift_1) ? blockShift_0 : blockShift_1;
    return signalDelay;
  }

  int calculateDistance(int signalDelay) {
    return (int)((signalDelay + PULSE_WIDTH_SAMPLES) * SAMPLE_PERIOD_US * 0.000001 * SOUND_SPEED_CM_PER_S * 0.5);
  }

  int calculateMinCorrelationSample(int signalDelay) {
    int max_sampels_between_mics = (int)ceil(((MIC_DIST_CM / SOUND_SPEED_CM_PER_S) * 1000000.0) / SAMPLE_PERIOD_US);
    int margin = 2 * max_sampels_between_mics;
    int min_correlation_sample = (signalDelay - margin) > 0 ? (signalDelay - margin) : 0;
    return min_correlation_sample;
  }

  int calculateMaxCorrelationSample(int signalDelay) {
    int max_correlation_samples = DATA_BUFFER - 1;
    int max_sampels_between_mics = (int)ceil(((MIC_DIST_CM / SOUND_SPEED_CM_PER_S) * 1000000.0) / SAMPLE_PERIOD_US);
    int margin = 2 * max_sampels_between_mics;
    int max_correlation_sample = (signalDelay + PULSE_WIDTH_SAMPLES + margin) < max_correlation_samples ? (signalDelay + PULSE_WIDTH_SAMPLES + margin) : max_correlation_samples;
    return max_correlation_sample;
  }

  int calculateCrossCorrelation(uint8_t (*buffer)[N_CHANNELS], int channel1, int channel2, int min_sample, int max_sample) {
    int max_correlation = INT_MIN;
    int max_correlation_idx = 0;

    for (int i = min_sample; i <= max_sample; i++) {
      int correlation = 0;
      for (int j = 0; j < PULSE_WIDTH_SAMPLES; j++) {
        correlation += buffer[i + j][channel1] * buffer[i + j][channel2];
      }
      if (correlation > max_correlation) {
        max_correlation = correlation;
        max_correlation_idx = i;
      }
    }

    return max_correlation_idx;
  }

  int calculateRelativeShift(int max_correlation_idx, int min_correlation_sample, int max_correlation_sample) {
    int relative_shift = max_correlation_idx - ((max_correlation_sample - min_correlation_sample - 1) * 0.5);
    return relative_shift;
  }

  int calculateAngleFromShift(int shift) {
    int delay_us = shift * SAMPLE_PERIOD_US;
    int smallest = INT_MAX;
    int sidx = -1;
    for (int i = 0; i < 181; i++) {
        int current = pow(angleLookup[i] - delay_us, 2);
        if (current < smallest) {
            smallest = current;
            sidx = i;
        }
    }
    return sidx - 90;
  }

  double deg2rad(double deg) {
    return deg * M_PI / 180.0;
  }
};
