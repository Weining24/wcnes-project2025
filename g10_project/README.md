# Backscatter Project

project description... 

## OVERVIEW OF PROJECT

    Carrier (Firefly): Generates a continuous-wave (CW) carrier signal.

    Backscatter (Pico): Uses the RP2040’s PIO to modulate the incoming carrier and send data packets.

    Receiver (TI CC1352): Receives and demodulates the backscattered signal using SmartRF Studio.

## Carrier Setup

python3 -m serial.tools.miniterm -e [your-port] 115200

Once connected, configure the carrier frequency. For example, to set the carrier at 2.45 GHz, type:

        f 2450


## Backscatter Setup

    Building & Flashing:

        Navigate to the project folder (e.g., backscatter_project).

        Create a build directory and compile:

        mkdir build
        cd build
        cmake ..
        make

## Receiver Setup

SmartRF config...