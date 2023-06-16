
#include <math.h>
#include <limits.h>
// #include "C:\CSharp\AAU\ESD2\embedded-integration\filter-c/filter.h"
// #include "C:\CSharp\AAU\ESD2\embedded-integration\filter-c/filter.c"
#include "T:\Repoes\AAU\ESD2\Project\embedded-integration\filter-c/filter.h"
#include "T:\Repoes\AAU\ESD2\Project\embedded-integration\filter-c/filter.c"

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
#define BAND_PASS_HIGH_CUT 38700
#define BAND_PASS_ORDER 8 // 4th order low and high pass

#define MAX(i, j) (((i) > (j)) ? (i) : (j))
#define MIN(i, j) (((i) < (j)) ? (i) : (j))

class AngleOfArrival {
public:
  AngleOfArrival() {
    calculateAngleLookupTable();
  }

  int calculateAngleAndDistance(uint8_t (*buffer)[N_CHANNELS], int* angle, int* distance_cm) {
    int used_channels[N_CHANNELS];
    int num_used_channels = findUsedChannels(buffer, used_channels);

    // Serial.print("Used channels: "); 
    // for (int i = 0; i < num_used_channels; i++){
    //   Serial.print(used_channels[i], DEC); Serial.print(", ");
    // }
    // Serial.println();

    // return error if under two active channels
    if (num_used_channels < 2) return 1;

    // process
    int signalDelay = fiilterAndFindSignalDelay(buffer, used_channels, num_used_channels);
    // signalDelay = 905;
    int max_correlation_sample = calculateMaxCorrelationSample(signalDelay);
    int min_correlation_sample = calculateMinCorrelationSample(signalDelay);
    int relative_shift = calculateCrossCorrelationFiltered(buffer, used_channels[0], used_channels[1], min_correlation_sample, max_correlation_sample);

    // Serial.print("signalDelay: "); Serial.println(signalDelay, DEC);
    // Serial.print("max_correlation_sample: "); Serial.println(max_correlation_sample, DEC);
    // Serial.print("min_correlation_sample: "); Serial.println(min_correlation_sample, DEC);
    // Serial.print("relative_shift: "); Serial.println(relative_shift, DEC);

    (*distance_cm) = calculateDistance(signalDelay);
    (*angle) = calculateAngleFromShift(relative_shift);

    return 0;
  }

private:
  int angleLookup[181];  // -90 to +90 degrees

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

  int fiilterAndFindSignalDelay(uint8_t (*buffer)[N_CHANNELS], int* used_channels, int num_used_channels) {
    float * filtered = (float *)malloc(PULSE_WIDTH_SAMPLES * sizeof(float));
    int minDelay = INT_MAX;

    for (int i = 0; i < num_used_channels; i++) {
      // initialize band pass filter
      BWBandPass* filter = create_bw_band_pass_filter(BAND_PASS_ORDER, SAMPEL_RATE, BAND_PASS_LOW_CUT, BAND_PASS_HIGH_CUT);

      // filter channel
      float max_correlationVal = 0;
      int delay = 0;
      float correlationVal = 0;
      for (int j = PULSE_WIDTH_SAMPLES, relative = 0; j < DATA_BUFFER; j++, relative++) {
        float tempFilteredCorr = ((float)bw_band_pass(filter, buffer[j][used_channels[i]]) * 255.0d);
        filtered[MIN(PULSE_WIDTH_SAMPLES-1, relative)] = tempFilteredCorr;
        correlationVal += tempFilteredCorr;

        // when buffer is filled, correlate signal block
        if (relative >= PULSE_WIDTH_SAMPLES-1) {
          if (correlationVal > max_correlationVal) {
            max_correlationVal = correlationVal;
            delay = relative + 1;
          }

          correlationVal -= filtered[0]; // remove the oldest correlation value

          // when buffer is filled, left-shift the array by one entry
          memcpy(filtered, filtered + 1, sizeof(float) * (PULSE_WIDTH_SAMPLES-1));
        }
      }

      // set smallest delay
      if (delay < minDelay) minDelay = delay;

      // free memory
      free_bw_band_pass(filter);
    }

    // free memory
    free(filtered);
    return minDelay;
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

  int calculateCrossCorrelationFiltered(uint8_t (*buffer)[N_CHANNELS], int channel1, int channel2, int min_sample, int max_sample) {
    int length = (max_sample-min_sample) + 1;
    float * filtered = (float *)malloc(sizeof(float) * 2 * length); // dynamic allocate buffer
    if (filtered == NULL) {
      Serial.println("malloc failed!");
      return 0;
    }

    // filter desired section
    BWBandPass* filter1 = create_bw_band_pass_filter(BAND_PASS_ORDER, SAMPEL_RATE, BAND_PASS_LOW_CUT, BAND_PASS_HIGH_CUT);
    BWBandPass* filter2 = create_bw_band_pass_filter(BAND_PASS_ORDER, SAMPEL_RATE, BAND_PASS_LOW_CUT, BAND_PASS_HIGH_CUT);

    for (int i = PULSE_WIDTH_SAMPLES, relative = (-min_sample + PULSE_WIDTH_SAMPLES); i <= max_sample; i++, relative++) {
      filtered[MAX(0, relative)*2] = bw_band_pass(filter1, buffer[i][channel1]);
      filtered[(MAX(0, relative)*2)+1] = bw_band_pass(filter2, buffer[i][channel2]);
    }

    // free memory
    free_bw_band_pass(filter1);
    free_bw_band_pass(filter2);

    // correlate section
    float max_correlation = INT_MIN;
    int max_correlation_idx = 0;

    int shift_count = (length - 1) * 0.5;

    for (int i = -shift_count; i <= shift_count; ++i) {
      float sum = 0;

      for (int j = 0; j < length - abs(i); ++j) {
        int idx1 = j + MAX(0, i);
        int idx2 = j + MAX(0, -i);

        sum += filtered[(idx1*2)+1] * filtered[(idx2*2)];
      }

      if (sum >= max_correlation) {
        max_correlation = sum;
        max_correlation_idx = i;            
      }
    }

    // clear buffer
    free(filtered);

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
