#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define PICO_PLATFORM rp2350 - arm - s

float tuned_frequency = 28074000.0;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
  double input_freq;
  double vco_max;
  double vco_min;
  double output_freq;
  int low_vco;
} PllParams;

void parse_args(PllParams *params) {
  // Set default values
  params->input_freq = 12e6;  // 12 MHz
  params->vco_max = 1600e6;   // 1600 MHz
  params->vco_min = 750e6;    // 750 MHz
  params->low_vco = 0;
}

void calculate_pll_params(PllParams *params, double *bestoutput) {
  double best_output = 0.0;
  int best_fbdiv = 0, best_pd1 = 0, best_pd2 = 0;
  double best_margin = params->output_freq;

  // Iterate through possible FBDIV values
  int start = params->low_vco ? 16 : 320;
  int end = params->low_vco ? 320 : 16;
  int step = params->low_vco ? 1 : -1;

  for (int fbdiv = start;
       params->low_vco ? (fbdiv <= end) : (fbdiv >= end);
       fbdiv += step) {
    double vco = params->input_freq * fbdiv;

    if (vco < params->vco_min || vco > params->vco_max) {
      continue;
    }

    // Try all possible post-divider combinations
    for (int pd2 = 1; pd2 <= 7; pd2++) {
      for (int pd1 = 1; pd1 <= 7; pd1++) {
        double out = vco / (pd1 * pd2);
        double margin = fabs(out - params->output_freq);

        if (margin < best_margin) {
          best_margin = margin;
          best_output = out;
          best_fbdiv = fbdiv;
          best_pd1 = pd1;
          best_pd2 = pd2;
        }
      }
    }
  }

  *bestoutput = best_output;

  // Print results in Hertz
  // printf("Requested: %.0f Hz\n", params->output_freq);
  // printf("Achieved: %.0f Hz\n", best_output);
  // printf("FBDIV: %d (VCO = %.0f Hz)\n", best_fbdiv, params->input_freq * best_fbdiv);
  // printf("PD1: %d\n", best_pd1);
  // printf("PD2: %d\n", best_pd2);
}

int main() {
  struct PLLSettings {
    uint32_t frequency;
    uint8_t refdiv;
    uint16_t fbdiv;
    uint8_t postdiv1;
    uint8_t postdiv2;
  };
  //A list of all the achievable frequencies in range
  PLLSettings possible_frequencies[] = {
    { 125000000, 1, 125, 6, 2 },
    { 125142857, 1, 73, 7, 1 },
    { 125333333, 1, 94, 3, 3 },
    { 126000000, 1, 126, 6, 2 },
    { 126666666, 1, 95, 3, 3 },
    { 126857142, 1, 74, 7, 1 },
    { 127000000, 1, 127, 6, 2 },
    { 127200000, 1, 106, 5, 2 },
    { 127500000, 1, 85, 4, 2 },
    { 128000000, 1, 128, 6, 2 },
    { 128400000, 1, 107, 5, 2 },
    { 128571428, 1, 75, 7, 1 },
    { 129000000, 1, 129, 6, 2 },
    { 129333333, 1, 97, 3, 3 },
    { 129600000, 1, 108, 5, 2 },
    { 130000000, 1, 130, 6, 2 },
    { 130285714, 1, 76, 7, 1 },
    { 130500000, 1, 87, 4, 2 },
    { 130666666, 1, 98, 3, 3 },
    { 130800000, 1, 109, 5, 2 },
    { 131000000, 1, 131, 6, 2 },
    { 132000000, 1, 132, 6, 2 },
    { 133000000, 1, 133, 6, 2 }
#if PICO_PLATFORM == rp2350 - arm - s
    ,
    { 133200000, 1, 111, 5, 2 },
    { 133333333, 1, 100, 3, 3 },
    { 133500000, 1, 89, 4, 2 },
    { 133714285, 1, 78, 7, 1 },
    { 134000000, 1, 67, 6, 1 },
    { 134400000, 1, 112, 5, 2 },
    { 134666666, 1, 101, 3, 3 },
    { 135000000, 1, 90, 4, 2 },
    { 135428571, 1, 79, 7, 1 },
    { 135600000, 1, 113, 5, 2 },
    { 136000000, 1, 102, 3, 3 },
    { 136500000, 1, 91, 4, 2 },
    { 136800000, 1, 114, 5, 2 },
    { 137142857, 1, 80, 7, 1 },
    { 137333333, 1, 103, 3, 3 },
    { 138000000, 1, 115, 5, 2 },
    { 138666666, 1, 104, 3, 3 },
    { 138857142, 1, 81, 7, 1 },
    { 139200000, 1, 116, 5, 2 },
    { 139500000, 1, 93, 4, 2 },
    { 140000000, 1, 105, 3, 3 },
    { 140400000, 1, 117, 5, 2 },
    { 140571428, 1, 82, 7, 1 },
    { 141000000, 1, 94, 4, 2 },
    { 141333333, 1, 106, 3, 3 },
    { 141600000, 1, 118, 5, 2 },
    { 142000000, 1, 71, 6, 1 },
    { 142285714, 1, 83, 7, 1 },
    { 142500000, 1, 95, 4, 2 },
    { 142666666, 1, 107, 3, 3 },
    { 142800000, 1, 119, 5, 2 },
    { 144000000, 1, 120, 5, 2 },
    { 145200000, 1, 121, 5, 2 },
    { 145333333, 1, 109, 3, 3 },
    { 145500000, 1, 97, 4, 2 },
    { 145714285, 1, 85, 7, 1 },
    { 146000000, 1, 73, 6, 1 },
    { 146400000, 1, 122, 5, 2 },
    { 146666666, 1, 110, 3, 3 },
    { 147000000, 1, 98, 4, 2 },
    { 147428571, 1, 86, 7, 1 },
    { 147600000, 1, 123, 5, 2 },
    { 148000000, 1, 111, 3, 3 },
    { 148500000, 1, 99, 4, 2 },
    { 148800000, 1, 124, 5, 2 },
    { 149142857, 1, 870, 7, 1 },
    { 149333333, 1, 112, 3, 3 },
    { 150000000, 1, 125, 5, 2 },
    { 270000000, 1, 90, 4, 1 },
    { 271200000, 1, 113, 5, 1 }
#endif
  };

  PllParams params;
  parse_args(&params);
  // calculate_pll_params(&params);

  float adjusted_frequency_up = tuned_frequency;    //+ 6000;
  float adjusted_frequency_down = tuned_frequency;  //- 6000;
  PLLSettings best_settings;
  float best_frequency = 1;
  float best_divider;
  float best_error = 1000000;
  best_frequency = 1;
  float best_best;
  // for(uint8_t idx = 0; idx < sizeof(possible_frequencies)/sizeof(PLLSettings); idx++)
  for (uint64_t i = 0; i < 500000000ULL; i += 100) {
    double system_clock_frequency;
    params.output_freq = 270000000 + i;
    calculate_pll_params(&params, &system_clock_frequency);
    //	    printf("%lf %lf XXX\n", params.output_freq, system_clock_frequency);

    //      float system_clock_frequency = possible_frequencies[idx].frequency;
    float ideal_divider = system_clock_frequency / (4.0f * adjusted_frequency_up);
    float nearest_divider = round(256.0f * ideal_divider) / 256.0f;
    float actual_frequency = system_clock_frequency / nearest_divider;
    float error = abs(actual_frequency - 4.0f * adjusted_frequency_up);
    if (error < best_error && error > 399) {
      best_frequency = actual_frequency;
      // best_settings = possible_frequencies[idx];
      //
      best_best = system_clock_frequency;
      best_divider = nearest_divider;
      best_error = error;
    }
    ideal_divider = system_clock_frequency / (4.0f * adjusted_frequency_down);
    nearest_divider = round(256.0f * ideal_divider) / 256.0f;
    actual_frequency = system_clock_frequency / nearest_divider;
    error = abs(actual_frequency - 4.0f * adjusted_frequency_down);

    //if(error < best_error)

    if (error < best_error && error > 399) {

      best_frequency = actual_frequency;
      // best_settings = possible_frequencies[idx];
      best_best = system_clock_frequency;
      best_divider = nearest_divider;
      best_error = error;
    }
  }
  //adjust system clock
  uint32_t vco_freq = (12000000 / best_settings.refdiv) * best_settings.fbdiv;
  //return actual frequency
  printf("%lf <--- %lf *** %lf\n", best_frequency / 4.0f, best_best, best_error / 4.0);
}
