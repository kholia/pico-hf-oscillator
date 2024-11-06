# SPDX-License-Identifier: MIT
LICENSE = """
Copyright 2022 Blazej Floch

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

TITLE = "FrequencyFinder"
VERSION = (0, 1, 0)

HELP = """
Finds the closest possible clocks to a given frequency for the Raspberry Pi Pico.
This tool is based on the `vcocalc.py` tool from the pico-sdk.

Make sure to set the PICO_SDK_PATH environment variable.

Columns Description:
    Error -> Absolute difference between requested (multiplied) and achieved frequency
    Mult -> Integer multiplier of the requested frequency
    Requested Freq -> Multiplied Requested Frequency (requested_freq * Mult)
    Achieved Freq -> Possible frequency
    Over-/Undercl -> Difference to given input frequency
    FBDIV -> Feedback Divider
    VCO -> Voltage Controller Oscillator frequency (fbdiv * crystal_freq_khz)
    PD1 -> First Post Divider
    PD2 -> Second Post Divider
"""

import argparse
import math
import subprocess
import os
import re
import sys

CALC_SCRIPT_PATH = os.path.join("./vcocalc.py")
if not os.path.isfile(CALC_SCRIPT_PATH):
    print("ERROR: Could not find vcocalc.py at:\n\t{}".format(CALC_SCRIPT_PATH), file=sys.stderr)
    sys.exit(1)

CALC_SCRIPT_CMD = [
    "python",
    CALC_SCRIPT_PATH
]

PAT_RESULT = re.compile(r'Achieved: (\d+[.]\d+) MHz\nFBDIV: (\d+) [(]VCO = (\d+) MHz[)]\nPD1: (\d+)\nPD2: (\d+)')


def get_achieved_frequency(requested_freq: float) -> dict:
    cmd = CALC_SCRIPT_CMD + [str(requested_freq)]
    result = subprocess.run(cmd, capture_output=True)
    if result.returncode != 0:
        print(result.stderr.decode(), file=sys.stderr, end='')
        sys.exit(result.returncode)
        return {}
    match = PAT_RESULT.findall(result.stdout.decode())
    if len(match) != 1:
        return {}

    return {
        "freq": float(match[0][0]),
        "fbdiv": int(match[0][1]),
        "vco": int(match[0][2]),
        "pd1": int(match[0][3]),
        "pd2": int(match[0][4]),
    }


def print_header():
    print(
        "{:>18}, {:>4}, {:>21}, {:>21}, {:>16}, {:>6}, {:>5}, {:>4}, {:>4}".format(
            "Error",
            "Mult",
            "Requested Freq",
            "Achieved Freq",
            "Over-/Undercl",
            "FBDIV",
            "VCO",
            "PD1",
            "PD2"
        )
    )
def print_entry(abs_error, default_freq, entry):
    print(
        "{:>18.16f}, {:>4}, {:>21.16f}, {:>21.16f}, {:>16.6f}, {:>6}, {:>5}, {:>4}, {:>4}".format(
            abs_error,
            entry['requested_multiplier'],
            entry['requested_freq'],
            entry['freq'],
            entry['freq'] - default_freq,
            entry['fbdiv'],
            entry['vco'],
            entry['pd1'],
            entry['pd2']
        )
    )

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=HELP)
    parser.add_argument('requested_freq', type=float, help="Desired frequency")
    parser.add_argument('--all', action='store_true', help="Print all result, by default the closest match(es) will be shown")
    parser.add_argument('--max_freq', type=float, default=420, help="The maximum frequency the CPU can be overclocked to")
    parser.add_argument('--min_freq', type=float, default=16, help="The minimum frequency the CPU can run with")
    parser.add_argument('--default_freq', type=float, default=125.0, help="The default CPU frequency used to calculate over-/underclock")
    parser.add_argument('--version', action='version', version='{} v{}'.format(TITLE, ".".join([str(s) for s in VERSION])) ,help='Show version and exit')

    args = parser.parse_args()

    min_multiplier = math.floor(args.min_freq / args.requested_freq)
    max_multiplier = math.floor(args.max_freq / args.requested_freq)
    default_freq = args.default_freq

    all = {}

    for i in range(min_multiplier, max_multiplier + 1):
        multiple_freq = i * args.requested_freq

        achieved = get_achieved_frequency(multiple_freq)

        if len(achieved) == 0:
             continue

        error = achieved['freq'] - multiple_freq

        if not error in all:
            all[abs(error)] = []

        achieved['requested_freq'] = multiple_freq
        achieved['error'] = error
        achieved['requested_multiplier'] = i

        all[abs(error)].append(achieved)

    if len(all) == 0:
        sys.exit(1)

    print_header()
    if not args.all:
        abs_error = sorted(all)[0]
        for entry in all[abs_error]:
            print_entry(abs_error, default_freq, entry)
    else:
        for abs_error in sorted(all):
            for entry in all[abs_error]:
                print_entry(abs_error, default_freq, entry)
