#include "nco.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <cmath>
#include <stdio.h>

float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out) {

  // We can get closer to the desired frequency if we allow small adjustments
  // to the system clock. system clocks in the range 125 - 133 MHz are fast
  // enough to run the software. There are nearly 50 different frequencies in
  // this range, by chosing the frequency that gives the best match, we can
  // get within about 4Khz.
  struct PLLSettings {
    uint32_t frequency;
    uint8_t refdiv;
    uint16_t fbdiv;
    uint8_t postdiv1;
    uint8_t postdiv2;
  };

  // Dhiru's tip: python frequency_finder.py --all --min_freq 50 --max_freq 350 28.074

  //A list of all the achievable frequencies in range
  PLLSettings possible_frequencies[] = {
    // {270000000, 1, 90, 4, 1},
    { 300000000, 1, 125, 5, 1 },
  };

  double adjusted_frequency_up = tuned_frequency + 6000;
  double adjusted_frequency_down = tuned_frequency - 6000;
  // double adjusted_frequency_up = tuned_frequency;
  // double adjusted_frequency_down = tuned_frequency;
  PLLSettings best_settings = { 0 };
  double best_frequency = 1.0;
  double best_divider = 0.0;
  double best_error = 1000000.0;

  for (uint8_t idx = 0; idx < sizeof(possible_frequencies) / sizeof(PLLSettings); idx++) {

    uint32_t system_clock_frequency = possible_frequencies[idx].frequency;

    double ideal_divider = system_clock_frequency / (4.0 * adjusted_frequency_up);
    double nearest_divider = round(256.0 * ideal_divider) / 256.0;
    double actual_frequency = system_clock_frequency / nearest_divider;
    double error = abs(actual_frequency - 4.0 * adjusted_frequency_up);
    if (error < best_error) {
      best_frequency = actual_frequency;
      best_settings = possible_frequencies[idx];
      best_divider = nearest_divider;
      best_error = error;
      system_clock_frequency_out = system_clock_frequency;
    }

    ideal_divider = system_clock_frequency / (4.0 * adjusted_frequency_down);
    nearest_divider = round(256.0 * ideal_divider) / 256.0;
    actual_frequency = system_clock_frequency / nearest_divider;
    error = abs(actual_frequency - 4.0 * adjusted_frequency_down);
    if (error < best_error) {
      best_frequency = actual_frequency;
      best_settings = possible_frequencies[idx];
      best_divider = nearest_divider;
      best_error = error;
      system_clock_frequency_out = system_clock_frequency;
    }
  }

  printf("system_clock_frequency_out -> %d\n", system_clock_frequency_out);

  assert(best_error < 1000000);
  //adjust system clock
  uint32_t vco_freq = (12000000 / best_settings.refdiv) * best_settings.fbdiv;
  set_sys_clock_pll(vco_freq, best_settings.postdiv1, best_settings.postdiv2);

  //set pio divider
  pio_sm_set_clkdiv(pio, sm, best_divider);

  //return actual frequency
  return best_frequency / 4.0;
}
