
#include <math.h>
#include <limits.h>
#include "C:\CSharp\AAU\ESD2\embedded-integration\filter-c/filter.h"
#include "C:\CSharp\AAU\ESD2\embedded-integration\filter-c/filter.c"
// #include "T:\Repoes\AAU\ESD2\Project\embedded-integration\filter-c/filter.h"
// #include "T:\Repoes\AAU\ESD2\Project\embedded-integration\filter-c/filter.c"

#define DATA_BUFFER 16000
#define N_CHANNELS 4
#define SOUND_SPEED_CM_PER_S 34300
#define MIC_DIST_CM 7.5
#define PULSE_WIDTH_MS 1

#define SAMPEL_RATE 80000
#define SAMPLE_PERIOD_US ((1.0 / SAMPEL_RATE) * 1e6)
#define SAMPLES_PER_MS (SAMPEL_RATE * 0.001)
#define PULSE_WIDTH_SAMPLES ((int)(PULSE_WIDTH_MS * SAMPLES_PER_MS))

// settings for digital filter
#define BAND_PASS_LOW_CUT 38200
#define BAND_PASS_HIGH_CUT 38700 // 39000
#define BAND_PASS_ORDER 5

#define MAX(i, j) (((i) > (j)) ? (i) : (j))

class AngleOfArrival {
public:
  AngleOfArrival() {
    calculateAngleLookupTable();
    memset(signal_block, 255, PULSE_WIDTH_SAMPLES);
    // for (int i = 0; i < PULSE_WIDTH_SAMPLES; i++)
    //   signal_block[i] = (i % 2 == 0) ? 255 : 0;
  }

  int calculateAngleAndDistance(uint8_t (*buffer)[N_CHANNELS], int* angle, int* distance_cm) {
    int used_channels[N_CHANNELS];
    int num_used_channels = findUsedChannels(buffer, used_channels);

    // return error if under two active channels
    if (num_used_channels < 2) return 1;

    // filter used channels
    filterUsedChannels(buffer, used_channels, num_used_channels);

    // process
    int signalDelay = calculateSignalDelay(buffer, used_channels[0], used_channels[1]);
    int max_correlation_sample = calculateMaxCorrelationSample(signalDelay);
    int min_correlation_sample = calculateMinCorrelationSample(signalDelay);
    int relative_shift = calculateCrossCorrelation(buffer, used_channels[0], used_channels[1], min_correlation_sample, max_correlation_sample);

    Serial.print("signalDelay: "); Serial.println(signalDelay, DEC);
    Serial.print("max_correlation_sample: "); Serial.println(max_correlation_sample, DEC);
    Serial.print("min_correlation_sample: "); Serial.println(min_correlation_sample, DEC);
    Serial.print("relative_shift: "); Serial.println(relative_shift, DEC);

    (*distance_cm) = calculateDistance(signalDelay);
    (*angle) = calculateAngleFromShift(relative_shift);

    return 0;
  }

private:
  int angleLookup[181];  // -90 to +90 degrees
  uint8_t signal_block[PULSE_WIDTH_SAMPLES];

  void calculateAngleLookupTable() {
    for (int i = -90; i <= 90; i++) {
      double distance = MIC_DIST_CM * sin(deg2rad(i));
      double delay_us = (distance / SOUND_SPEED_CM_PER_S) * 1.0e6;
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
          break;
        }
      } 
    }
    return num_used_channels;
  }

  void filterUsedChannels(uint8_t (*buffer)[N_CHANNELS], int* used_channels, int num_used_channels) {
    for (int i = 0; i < num_used_channels; i++) {
      // initialize band pass filter
      BWBandPass* filter = create_bw_band_pass_filter(BAND_PASS_ORDER, SAMPEL_RATE, BAND_PASS_LOW_CUT, BAND_PASS_HIGH_CUT);

      // filter channel
      for (int j = 0; j < DATA_BUFFER; j++) {
        int temp = (127 + bw_band_pass(filter, buffer[j][used_channels[i]]));
        buffer[j][used_channels[i]] = (uint8_t)temp;

        if (temp < 0 || temp > 255) {
          Serial.print("filter: "); Serial.println(temp, DEC);
        }
      }

      // free memory
      free_bw_band_pass(filter);
    }
  }

  int calculateSignalDelay(uint8_t (*buffer)[N_CHANNELS], int channel1, int channel2) {
    int blockShift_0 = 0;
    int blockShift_1 = 0;
    int max_correlation_0 = 0;
    int max_correlation_1 = 0;

    for (int i = PULSE_WIDTH_SAMPLES; i < DATA_BUFFER - PULSE_WIDTH_SAMPLES; i++) {
      int correlation_0 = 0;
      int correlation_1 = 0;            
      for (int j = 0; j < PULSE_WIDTH_SAMPLES; j++) {
        correlation_0 += ((int)buffer[i + j][channel1] - 127) * signal_block[j];
        correlation_1 += ((int)buffer[i + j][channel2] - 127) * signal_block[j];
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

    Serial.print("blockShift_0: "); Serial.println(blockShift_0, DEC);
    Serial.print("blockShift_1: "); Serial.println(blockShift_1, DEC);

    int signalDelay = (blockShift_0 < blockShift_1) ? blockShift_0 : blockShift_1;
    return signalDelay;
  }

  int calculateDistance(int signalDelay) {
    return (int)(signalDelay * SAMPLE_PERIOD_US * 0.000001 * SOUND_SPEED_CM_PER_S * 0.5);
  }

  int calculateMinCorrelationSample(int signalDelay) {
    int max_sampels_between_mics = (int)ceil(((MIC_DIST_CM / SOUND_SPEED_CM_PER_S) * 1.0e6) / SAMPLE_PERIOD_US);
    int margin = 2 * max_sampels_between_mics;
    int min_correlation_sample = (signalDelay - margin);
    if (min_correlation_sample < 0) min_correlation_sample = 0;
    return min_correlation_sample;
  }

  int calculateMaxCorrelationSample(int signalDelay) {
    int max_correlation_samples = DATA_BUFFER - 1;
    int max_sampels_between_mics = (int)ceil(((MIC_DIST_CM / SOUND_SPEED_CM_PER_S) * 1.0e6) / SAMPLE_PERIOD_US);
    int margin = 2 * max_sampels_between_mics;
    int max_correlation_sample = (signalDelay + PULSE_WIDTH_SAMPLES + margin);
    if (max_correlation_sample > max_correlation_samples) max_correlation_sample = max_correlation_samples;
    return max_correlation_sample;
  }

  int calculateCrossCorrelation(uint8_t (*buffer)[N_CHANNELS], int channel1, int channel2, int min_sample, int max_sample) {
    int correlation_length = (max_sample - min_sample) + 1;
    int max_correlation = INT_MIN;
    int max_correlation_idx = 0;

    int shift_count = correlation_length * 0.5;

    for (int i = -shift_count; i <= shift_count; ++i) {
        int sum = 0;

        for (int j = 0; j < correlation_length - abs(i); ++j) {
            int idx1 = min_sample + j + MAX(0, i);
            int idx2 = min_sample + j + MAX(0, -i);

            if (idx1 < min_sample || idx2 < min_sample || idx1 > max_sample || idx2 > max_sample) {
              Serial.println("correlation boundery");
              continue;
            }

            sum += (((int)buffer[idx1][channel1]) - 127) * (((int)buffer[idx2][channel2]) - 127);
        }

        if (sum >= max_correlation) {
            max_correlation = sum;
            max_correlation_idx = i;            
        }
    }

    return max_correlation_idx;
  }

  int calculateAngleFromShift(int shift) {
    int delay_us = shift * SAMPLE_PERIOD_US;
    int smallest_error = INT_MAX;
    int sidx = -1;
    for (int i = 0; i < 181; i++) {
        int current_error = pow(angleLookup[i] - delay_us, 2);
        if (current_error < smallest_error) {
            smallest_error = current_error;
            sidx = i;
        }
    }
    return sidx - 90;
  }

  double deg2rad(double deg) {
    return deg * M_PI / 180.0;
  }
};
