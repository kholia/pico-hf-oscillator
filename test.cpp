///////////////////////////////////////////////////////////////////////////////
//
//  Roman Piksaykin [piksaykin@gmail.com], R2BDY
//  https://www.qrz.com/db/r2bdy
//
///////////////////////////////////////////////////////////////////////////////
//
//
//  test.c - Simple tests of digital controlled radio freq oscillator.
//
//
//  DESCRIPTION
//
//      The oscillator provides precise generation of any frequency ranging
//  from 1 Hz to 33.333 MHz with tenth's of millihertz resolution (please note that
//  this is relative resolution owing to the fact that the absolute accuracy of
//  onboard crystal of pi pico is limited; the absoulte accuracy can be provided
//  when using GPS reference option included).
//      The DCO uses phase locked loop principle programmed in C and PIO asm.
//      The DCO does *NOT* use any floating point operations - all time-critical
//  instructions run in 1 CPU cycle.
//      Currently the upper freq. limit is about 33.333 MHz and it is achieved only
//  using pi pico overclocking to 270 MHz.
//      Owing to the meager frequency step, it is possible to use 3, 5, or 7th
//  harmonics of generated frequency. Such solution completely cover all HF and
//  a portion of VHF band up to about 233 MHz.
//      Unfortunately due to pure digital freq.synthesis principle the jitter may
//  be a problem on higher frequencies. You should assess the quality of generated
//  signal if you want to emit a noticeable power.
//      This is an experimental project of amateur radio class and it is devised
//  by me on the free will base in order to experiment with QRP narrowband
//  digital modes.
//      I appreciate any thoughts or comments on this matter.
//
//  PROJECT PAGE
//      https://github.com/RPiks/pico-hf-oscillator
//
//  LICENCE
//      MIT License (http://www.opensource.org/licenses/mit-license.php)
//
//  Copyright (c) 2023 by Roman Piksaykin
//
//  Permission is hereby granted, free of charge,to any person obtaining a copy
//  of this software and associated documentation files (the Software), to deal
//  in the Software without restriction,including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY,WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "pico/stdlib.h"
#include <stdio.h>

#include "defines.h"
#include "pico.h"

extern "C" {
  #include "piodco/piodco.h"
  #include "dco2.pio.h"
}

#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "pico/stdio/driver.h"

#include "./lib/assert.h"
#include "./debug/logutils.h"
#include "hwdefs.h"

#include <GPStime.h>

#include <hfconsole.h>

#include "protos.h"

#include "nco.h"
#include "nco.pio.h"

//#define GEN_FRQ_HZ 32333333L
// #define GEN_FRQ_HZ 28023000L
// #define GEN_FRQ_HZ 14000000L
#define GEN_FRQ_HZ 28074000L

PioDco DCO; /* External in order to access in both cores. */

// https://www.codebug.org.uk/learn/step/541/morse-code-timing-rules/
const uint DOT_PERIOD_MS = 92;  // 13 WPM

const char *morse_letters[] = {
  ".-",    // A
  "-...",  // B
  "-.-.",  // C
  "-..",   // D
  ".",     // E
  "..-.",  // F
  "--.",   // G
  "....",  // H
  "..",    // I
  ".---",  // J
  "-.-",   // K
  ".-..",  // L
  "--",    // M
  "-.",    // N
  "---",   // O
  ".--.",  // P
  "--.-",  // Q
  ".-.",   // R
  "...",   // S
  "-",     // T
  "..-",   // U
  "...-",  // V
  ".--",   // W
  "-..-",  // X
  "-.--",  // Y
  "--.."   // Z
};

const char *morse_numbers[] = {
  "-----",  // 0
  ".----",  // 1
  "..---",  // 2
  "...--",  // 3
  "....-",  // 4
  ".....",  // 5
  "-....",  // 6
  "--...",  // 7
  "---..",  // 8
  "----."   // 9
};

void on() {
  PioDCOStart(&DCO);
}

void off() {
  PioDCOStop(&DCO);
  gpio_init(DCO._gpio);
  pio_gpio_init(DCO._pio, DCO._gpio);
}

void put_morse_letter(const char *pattern) {
  puts(pattern);
  for (; *pattern; ++pattern) {
    on();
    if (*pattern == '.')
      sleep_ms(DOT_PERIOD_MS);
    else
      sleep_ms(DOT_PERIOD_MS * 3);
    off();
    sleep_ms(DOT_PERIOD_MS * 1);
  }
  sleep_ms(DOT_PERIOD_MS * 2);
}

void put_morse_str(const char *str) {
  for (; *str; ++str) {
    printf("%c\n", *str);
    if (*str >= 'A' && *str <= 'Z') {
      put_morse_letter(morse_letters[*str - 'A']);
    } else if (*str >= 'a' && *str <= 'z') {
      put_morse_letter(morse_letters[*str - 'a']);
    } else if (*str >= '0' && *str <= '9') {
      put_morse_letter(morse_numbers[*str - '0']);
    } else if (*str == '?') {
      put_morse_letter("..--..");  // k1te ? Question mark
    } else if (*str == '/') {
      put_morse_letter("-..-.");  // k1te ~ for /B
    } else if (*str == '~') {
      put_morse_letter("..--");  // k1te ~ THE 'pi' SYMBOL (new 10/01/2022)
    } else if (*str == ' ') {
      sleep_ms(DOT_PERIOD_MS * 4);
    }
  }
}

/* This is the code of dedicated core.
   We deal with extremely precise real-time task. */
void core1_entry() {
  const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;

  /* Initialize DCO */
  assert_(0 == PioDCOInit(&DCO, 6, clkhz));

  /* Run DCO. */
  PioDCOStart(&DCO);

  /* Set initial freq. */
  assert_(0 == PioDCOSetFreq(&DCO, GEN_FRQ_HZ, 0u));

  /* Run the main DCO algorithm. It spins forever. */
  PioDCOWorker2(&DCO);
}

int main() {
  const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;
  // Choose which PIO instance to use (there are two instances)
  PIO pio;
  uint offset;
  uint sm;

  // set_sys_clock_khz(clkhz / 1000L, true);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  // configure SMPS into power save mode
  const uint PSU_PIN = 23;
  gpio_init(PSU_PIN);
  gpio_set_function(PSU_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(PSU_PIN, GPIO_OUT);
  gpio_put(PSU_PIN, 1);

  // configure PIO to act as quadrature oscillator
  pio = pio0;
  offset = pio_add_program(pio, &nco_program);
  sm = pio_claim_unused_sm(pio, true);
  nco_program_init(pio, sm, offset);
  double tuned_frequency_Hz = 28074000;
  double nco_frequency_Hz;
  double offset_frequency_Hz;
  uint32_t system_clock_rate;

  sleep_ms(5000);

  nco_frequency_Hz = nco_set_frequency(pio, sm, tuned_frequency_Hz, system_clock_rate);
  offset_frequency_Hz = tuned_frequency_Hz - nco_frequency_Hz;

  stdio_init_all();

  multicore_launch_core1(core1_entry);

  while (1) {
    put_morse_str("CQ CQ CQ DE VU3CER TEST BEACON");
    sleep_ms(1000);
    printf("%lf\n", offset_frequency_Hz);
  }
}
