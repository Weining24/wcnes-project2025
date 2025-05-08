import os
import re
import numpy as np
import pandas as pd
from functions import *  # Import functions from the .py file
from pylab import rcParams

# Set up figure size for plots
rcParams["figure.figsize"] = 16, 4

# Constants
PAYLOADSIZE = 14
if PAYLOADSIZE % 2 != 0:
    print("Alarm! The payload size is not even.")
NUM_16RND = (PAYLOADSIZE - 2) // 2  # How many 16-bit random numbers are included in each frame
MAX_SEQ = 256  # Maximum sequence number defined by the length of the seq, the length of seq is 1B

# Folder containing the data files
folder_path = 'data/testday'


packet_loss_dict = {}


# PARSES THE FILE NAME WITH THE SPECIFIC FORMAT AND EXTRACTS FREQUENCY
# FORMAT : NAME_FREQ_DECIMAL_TESTX
def get_freq_from_filename(filename):

    split_lst = filename.split("_")
    
    print(f"whole: {split_lst[1]}, d: {split_lst[2]}")

    int_str = split_lst[1] + "." + split_lst[2]
    frequency = float(int_str)

    return frequency

idx = 0
for filename in os.listdir(folder_path):
    file_path = os.path.join(folder_path, filename)
    print(f"\n\n____________Processing file: {file_path}____________\n")

    # Skip empty files
    if os.path.getsize(file_path) == 0:
        print("Empty file, skipping.\n")
        continue

    # Import file into dataframe
    df = readfile("./" + file_path)
    df.head(10)  # Check the first 10 lines

    # Delete packets of invalid length
    test = df[df.payload.apply(lambda x: len(x) == (PAYLOADSIZE * 3 - 1))]
    test.reset_index(inplace=True)

    # Compute the file delay
    file_delay = df.time_rx[len(df) - 1] - df.time_rx[0]
    file_delay_s = np.timedelta64(file_delay, "ms").astype(int) / 1000
    print(f"The time it takes to transfer the file is : {file_delay}, which is {file_delay_s} seconds.")

    # Compute the BER for all received packets
    ber = compute_ber(test, PACKET_LEN=NUM_16RND * 2)
    bit_reliability = (1 - ber) * 100
    print(f"Bit error rate [%]: {(ber * 100):.8f}\t\t(in received packets within pseudo sequence + payload)")
    if len(df) < 1:
        print("Data rate [bit/s]: More than one packet required for analysis.")
    else:
        print(f"Data rate [bit/s]: {(len(df) * NUM_16RND * 2 / file_delay_s):.8f}\t\t(directly impacted by missed packets)")

    packet_loss = compute_packet_loss(df)
    per = compute_packet_error_rate(df)
    avg_rssi = compute_average_rssi(df)

    print(f"Packet loss: {packet_loss * 100:.2f}%")
    print(f"Packet error rate: {per * 100:.2f}%")
    print(f"Average RSSI: {avg_rssi}")

    # Extract frequency from the filename
    frequency = get_freq_from_filename(filename)
    if frequency is not None:
        packet_loss_dict[frequency] = packet_loss * 100

    idx += 1

# Print the packet loss results
print(f'Packet loss dictionary: {packet_loss_dict}')
