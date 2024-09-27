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
//  from 1 Hz to 33.333 MHz with tenth's of millihertz resolution (please note
//  that this is relative resolution owing to the fact that the absolute
//  accuracy of onboard crystal of pi pico is limited; the absoulte accuracy can
//  be provided when using GPS reference option included).
//      The DCO uses phase locked loop principle programmed in C and PIO asm.
//      The DCO does *NOT* use any floating point operations - all time-critical
//  instructions run in 1 CPU cycle.
//      Currently the upper freq. limit is about 33.333 MHz and it is achieved
//      only
//  using pi pico overclocking to 270 MHz.
//      Owing to the meager frequency step, it is possible to use 3, 5, or 7th
//  harmonics of generated frequency. Such solution completely cover all HF and
//  a portion of VHF band up to about 233 MHz.
//      Unfortunately due to pure digital freq.synthesis principle the jitter
//      may
//  be a problem on higher frequencies. You should assess the quality of
//  generated signal if you want to emit a noticeable power.
//      This is an experimental project of amateur radio class and it is devised
//  by me on the free will base in order to experiment with QRP narrowband
//  digital modes.
//      I appreciate any thoughts or comments on this matter.
//
//  TESTS LIST
//
//  SpinnerMFSKTest         - It generates a random sequence of 2-FSK stream.
//  SpinnerSweepTest        - Frequency sweep test of 5 Hz step.
//  SpinnerRTTYTest         - Random RTTY sequence test (170 Hz).
//  SpinnerMilliHertzTest   - A test of millihertz resolution of freq.setting.
//  SpinnerWide4FSKTest     - Some `wide` 4-FSK test (100 Hz per step, 400 Hz
//  overall). SpinnerGPSreferenceTest - GPS receiver connection and working
//  test.
//
//  PLATFORM
//      Raspberry Pi pico.
//
//  REVISION HISTORY
//
//      Rev 0.1   05 Nov 2023   Initial release
//      Rev 0.2   18 Nov 2023
//      Rev 1.0   10 Dec 2023   Improved frequency range (to ~33.333 MHz).
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
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "pico.h"

#include "dco2.pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdio/driver.h"
#include "piodco/piodco.h"

#include "./debug/logutils.h"
#include "./lib/assert.h"
#include "hwdefs.h"

#include <GPStime.h>

#include <hfconsole.h>

#include "protos.h"

// #define GEN_FRQ_HZ 32333333L
#define GEN_FRQ_HZ 28074000L  // 10m FT8

PioDco DCO; /* External in order to access in both cores. */

int main() {
  const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;
  set_sys_clock_khz(clkhz / 1000L, true);

  stdio_init_all();
  sleep_ms(1000);
  printf("Start\n");

  HFconsoleContext *phfc = HFconsoleInit(-1, 0);
  HFconsoleSetWrapper(phfc, ConsoleCommandsWrapper);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  multicore_launch_core1(core1_entry);

  for (;;) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(5);
    int r = HFconsoleProcess(phfc, 10);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(1);
  }

  for (;;) {
    sleep_ms(100);
    int chr = getchar_timeout_us(100); // getchar();
    printf("%d %c\n", chr, (char)chr);
  }

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  multicore_launch_core1(core1_entry);

  while (1) {
    sleep_ms(100);
  }

  // See https://github.com/RPiks/pico-hf-oscillator/blob/main/test.c
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
