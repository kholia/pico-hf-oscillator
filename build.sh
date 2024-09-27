#!/bin/bash

cd build
cmake -DPICO_BOARD=pico2 ..
make
