# Rust Implementation of O-QPSK on backscatter platform


the folder `pico_qpsk` has the code to run an implementation of O-QPSK that works with the TI Launchpad and Firefly

the folder `pico_probe_firmware` has a firmware image to use another pico as a debug probe,
this is required to get debug output from pico when running the rust program

btw to inspect the digital signals you can use [this project](https://github.com/dotcypress/ula)

if you set it to 200MHz with the max sample rate you can get data, 
but you can't measure anything less than like 8 cycles with the Maker pico, 
but the normal pico showed a bit better resolution 
