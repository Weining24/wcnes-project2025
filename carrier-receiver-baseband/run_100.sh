#!/bin/bash

# Check if output filename is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <output_filename>"
    exit 1
fi

OUTPUT_FILE="$1"

# Reboot into BOOTSEL mode
picotool reboot -uf
sleep 5

# Load the ELF file
picotool load build/carrier_receiver_baseband.elf
picotool reboot
sleep 2

# Capture serial output to specified file
picocom -b 115200 /dev/ttyACM0 | tee "$OUTPUT_FILE"

