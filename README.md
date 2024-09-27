# Info

This project provides a `Custom Quartz` (clock signal) using a Pico 2 MCU
board.

# Steps

```
mkdir -p ~/repos

cd ~/repos

git clone --recursive https://github.com/raspberrypi/pico-sdk.git

export PICO_SDK_PATH=$HOME/repos/pico-sdk

cmake .; make -j4
```

Copy `pico-wspr-tx.uf2` to Pico 2 as usual.
